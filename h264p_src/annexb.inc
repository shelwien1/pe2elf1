// The byte stream format (Annex B): start codes and emulation prevention,
// and the reader that splits a stream into its NAL units (t and c), which
// h264split.inc's splitter (mt.inc) shares.

namespace h264p {

// The RBSP of a NAL unit's payload (into r): its emulation prevention
// bytes removed
void unescape(const uint8_t* p, size_t n, Buf<uint8_t>& r) {
  r.clear();
  r.reserve(n);
  uint8_t* q = r.data();
  int zeros = 0;
  for (size_t i = 0; i < n; i++) {
    if (zeros >= 2 && p[i] == 3) {
      zeros = 0;
      continue;
    }
    *q++ = p[i];
    zeros = p[i] ? 0 : zeros + 1;
  }
  r.n = (size_t)(q - r.data());
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

// and back (appended to e): emulation prevention bytes where the standard
// needs them (one after a final zero byte too), as an encoder writes them
void escape(const uint8_t* r, size_t n, Buf<uint8_t>& e) {
  e.room(e.size() + n + n / 64 + 2);
  int zeros = 0;
  for (size_t i = 0; i < n; i++) {
    uint8_t b = r[i];
    if (zeros >= 2 && b <= 3) {
      e.push_back(3);
      zeros = 0;
    }
    e.push_back(b);
    zeros = b ? 0 : zeros + 1;
  }
  if (n && r[n - 1] == 0) e.push_back(3);
}

// A start code of 4 bytes (zero_byte, then 00 00 01)
const uint8_t kStartCode4[4] = {0, 0, 0, 1};
// Where in the n bytes at b the parameter sets copied into a part of -t go
// (h264split.inc) and are found again (mt.inc): after the access unit
// delimiter, up to its last nonzero byte, if the first NAL unit is one;
// else at the start
size_t aud_end(const uint8_t* b, size_t n) {
  size_t sc = find_start_code(b, n, 0);
  if (sc + 4 > n || (b[sc + 3] & 31) != 9) return 0;
  size_t j = Min(find_start_code(b, n, sc + 4), n);
  while (j > sc + 4 && !b[j - 1]) j--;
  return j;
}

// Splits an Annex B byte stream into NAL units while reading it.
struct AnnexBReader {
  void Init(ByteSource src) {
    src_ = src;
    buf_.Init();
    junk_.Init();
    head_ = scan_ = 0;
    zeros_ = eof_zeros_ = 0;
    started_ = done_ = false;
  }
  void Quit() {
    buf_.Quit();
    junk_.Quit();
  }

  // The next NAL unit: the 0x00 bytes before its start code (zero_byte, and
  // the trailing_zero_8bits of the previous NAL unit), and its bytes up to
  // the zero bytes before the next start code. False at the end.
  bool next(size_t& zeros, Buf<uint8_t>& nal) {
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
      junk_.assign(buf_.data(), j);
      zeros_ = k - j;
      started_ = true;
      head_ = scan_ = k + 3;
    }
    bool last = !find(k);
    if (last) k = buf_.size();
    size_t e = k;
    while (e > head_ && buf_[e - 1] == 0) e--;
    nal.assign(buf_.data() + head_, e - head_);
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
  const Buf<uint8_t>& junk() const { return junk_; }
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
      buf_.erase_front(head_);
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
  Buf<uint8_t> buf_, junk_;
  size_t head_;  // start of the current NAL unit in buf_
  size_t scan_;  // where to look for the next start code
  size_t zeros_, eof_zeros_;
  bool started_, done_;
};

}  // namespace h264p
