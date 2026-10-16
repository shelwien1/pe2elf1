// Bit-level reading and writing of RBSP data (emulation prevention removed).
#ifndef H264P_BITS_H
#define H264P_BITS_H

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace h264p {

// The input stream cannot be parsed (syntax error or unsupported feature):
// the NAL unit is then stored verbatim.
struct ParseError : std::runtime_error {
  explicit ParseError(const std::string& s) : std::runtime_error(s) {}
};

// The TSV file is malformed (only raised when restoring).
struct TsvError : std::runtime_error {
  explicit TsvError(const std::string& s) : std::runtime_error(s) {}
};

class BitReader {
 public:
  BitReader(const uint8_t* data, size_t size) : data_(data), nbits_(size * 8), pos_(0) {
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
    if (pos_ >= nbits_) throw ParseError("unexpected end of NAL unit");
    int b = (data_[pos_ >> 3] >> (7 - (pos_ & 7))) & 1;
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
      int b = p < nbits_ ? (data_[p >> 3] >> (7 - (p & 7))) & 1 : 0;
      v = (v << 1) | b;
    }
    return v;
  }
  void skip(int n) {
    if (pos_ + n > nbits_) throw ParseError("unexpected end of NAL unit");
    pos_ += n;
  }
  uint32_t ue() {
    int lz = 0;
    while (!bit())
      if (++lz > 31) throw ParseError("invalid Exp-Golomb code");
    return lz ? (uint32_t)((1ull << lz) - 1 + u(lz)) : 0;
  }
  int64_t se() {
    uint32_t k = ue();
    return (k & 1) ? (int64_t)(k / 2) + 1 : -(int64_t)(k / 2);
  }

 private:
  const uint8_t* data_;
  size_t nbits_, pos_;
  int64_t last_one_;
};

class BitWriter {
 public:
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
  void align_zero() {
    while (n_ & 7) bit(0);
  }
  size_t pos() const { return n_; }
  int at(size_t i) const {
    if (i >> 3 < buf_.size()) return (buf_[i >> 3] >> (7 - (i & 7))) & 1;
    return (cur_ >> ((n_ & 7) - 1 - (i & 7))) & 1;
  }
  void truncate(size_t nbits) {
    if (nbits >= n_) return;
    if (nbits >> 3 == buf_.size())  // within the incomplete byte
      cur_ = (uint8_t)(cur_ >> ((n_ & 7) - (nbits & 7)));
    else
      cur_ = (nbits & 7) ? (uint8_t)(buf_[nbits >> 3] >> (8 - (nbits & 7))) : 0;
    buf_.resize(nbits >> 3);
    n_ = nbits;
  }
  // whole bytes, the last one padded with zero bits
  const std::vector<uint8_t>& data() {
    if (n_ & 7) {
      std::vector<uint8_t> v(buf_);
      v.push_back((uint8_t)(cur_ << (8 - (n_ & 7))));
      padded_.swap(v);
      return padded_;
    }
    return buf_;
  }
  // appends whole bytes; the bit position must be byte aligned
  void append(size_t n, uint8_t value) { buf_.insert(buf_.end(), n, value), n_ += 8 * n; }

 private:
  std::vector<uint8_t> buf_, padded_;  // complete bytes
  uint8_t cur_ = 0;                    // the bits of the incomplete byte
  size_t n_ = 0;
};

}  // namespace h264p

#endif
