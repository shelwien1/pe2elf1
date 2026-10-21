// The byte stream format (Annex B): start codes and emulation prevention,
// and the reader that splits a stream into its NAL units (t and c), which
// h264split.inc's splitter (mt.inc) shares.

namespace h264p {

// The RBSP of a NAL unit's payload: its emulation prevention bytes removed
std::vector<uint8_t> unescape(const uint8_t* p, size_t n) {
  std::vector<uint8_t> r;
  r.reserve(n);
  int zeros = 0;
  for (size_t i = 0; i < n; i++) {
    if (zeros >= 2 && p[i] == 3) {
      zeros = 0;
      continue;
    }
    r.push_back(p[i]);
    zeros = p[i] ? 0 : zeros + 1;
  }
  return r;
}

// the first start code (00 00 01) in b[0, n) at i or after, n if none
inline size_t find_start_code(const uint8_t* b, size_t n, size_t i) {
  while (i + 2 < n) {
    const uint8_t* q = (const uint8_t*)memchr(b + i + 2, 1, n - i - 2);
    if (!q) return n;
    size_t p = (size_t)(q - b);
    if (!b[p - 1] && !b[p - 2]) return p - 2;
    i = p - 1;  // (the next one has its 01 after this one's)
  }
  return n;
}

// and back: emulation prevention bytes where the standard needs them (one
// after a final zero byte too), as an encoder writes them
std::vector<uint8_t> escape(const std::vector<uint8_t>& r) {
  std::vector<uint8_t> e;
  e.reserve(r.size() + r.size() / 64 + 2);
  int zeros = 0;
  for (uint8_t b : r) {
    if (zeros >= 2 && b <= 3) {
      e.push_back(3);
      zeros = 0;
    }
    e.push_back(b);
    zeros = b ? 0 : zeros + 1;
  }
  if (!r.empty() && r.back() == 0) e.push_back(3);
  return e;
}

// Splits an Annex B byte stream into NAL units while reading it.
struct AnnexBReader {
  explicit AnnexBReader(ByteSource src) : src_(src) {}

  // The next NAL unit: the 0x00 bytes before its start code (zero_byte, and
  // the trailing_zero_8bits of the previous NAL unit), and its bytes up to
  // the zero bytes before the next start code. False at the end.
  bool next(size_t& zeros, std::vector<uint8_t>& nal) {
    if (done_) return false;
    size_t k;
    if (!started_) {
      if (!find(k)) {  // no start code at all
        junk_.swap(buf_);
        done_ = true;
        return false;
      }
      size_t j = k;
      while (j > 0 && buf_[j - 1] == 0) j--;
      junk_.assign(buf_.begin(), buf_.begin() + j);
      zeros_ = k - j;
      started_ = true;
      head_ = scan_ = k + 3;
    }
    bool last = !find(k);
    if (last) k = buf_.size();
    size_t e = k;
    while (e > head_ && buf_[e - 1] == 0) e--;
    nal.assign(buf_.begin() + head_, buf_.begin() + e);
    zeros = zeros_;
    if (last) {
      eof_zeros_ = k - e;
      done_ = true;
    } else {
      zeros_ = k - e;
      head_ = scan_ = k + 3;
    }
    return true;
  }
  // bytes before the first start code (all bytes if there is none)
  const std::vector<uint8_t>& junk() const { return junk_; }
  // 0x00 bytes after the last NAL unit
  size_t eof_zeros() const { return eof_zeros_; }

  // the next start code at or after scan_, reading as much as needed
  bool find(size_t& k) {
    for (;;) {
      k = find_start_code(buf_.data(), buf_.size(), scan_);
      if (k < buf_.size()) return true;
      scan_ = buf_.size() > 2 ? buf_.size() - 2 : 0;
      if (scan_ < head_) scan_ = head_;
      // drop what was returned before reading more
      buf_.erase(buf_.begin(), buf_.begin() + head_);
      scan_ -= head_;
      head_ = 0;
      size_t old = buf_.size();
      buf_.resize(old + (1 << 16));
      size_t n = src_.read(&buf_[old], 1 << 16);
      buf_.resize(old + n);
      if (!n) return false;
    }
  }

  ByteSource src_;
  std::vector<uint8_t> buf_, junk_;
  size_t head_ = 0;  // start of the current NAL unit in buf_
  size_t scan_ = 0;  // where to look for the next start code
  size_t zeros_ = 0, eof_zeros_ = 0;
  bool started_ = false, done_ = false;
};

}  // namespace h264p
