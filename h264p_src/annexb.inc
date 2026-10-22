// The byte stream format (Annex B): start codes and emulation prevention,
// and the reader that splits a stream into its NAL units (c); the start
// codes and the access unit delimiter's end are h264split.inc's splitter's
// too (mt.inc).

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
// (h264split.inc; the part's record says where): after the access unit
// delimiter, up to its last nonzero byte, if the first NAL unit is one;
// else at the start
size_t aud_end(const uint8_t* b, size_t n) {
  size_t sc = find_start_code(b, n, 0);
  if (sc + 4 > n || (b[sc + 3] & 31) != 9) return 0;
  size_t j = Min(find_start_code(b, n, sc + 4), n);
  while (j > sc + 4 && !b[j - 1]) j--;
  return j;
}

// CRC-32 (IEEE) of a stream (c reads it, d writes it), coded at the end of a
// compressed one
struct Crc32 {
  void Init() {
    for (uint32_t i = 0; i < 256; i++) {
      uint32_t x = i;
      for (int k = 0; k < 8; k++) x = x & 1 ? 0xEDB88320u ^ (x >> 1) : x >> 1;
      t[i] = x;
    }
    c = 0xFFFFFFFFu;
  }
  void add(const uint8_t* p, size_t n) {
    for (size_t i = 0; i < n; i++) c = t[(c ^ p[i]) & 255] ^ (c >> 8);
  }
  void add(const Buf<uint8_t>& v) { add(v.data(), v.size()); }
  uint32_t value() const { return ~c; }
  uint32_t t[256];
  uint32_t c;
};

// Splits an Annex B byte stream into its segments while reading it: the
// bytes before the first start code (the junk), then each NAL unit's, each
// up to its last nonzero byte; the zero bytes after one go with the next
// start code (zero_byte, trailing_zero_8bits) or with the end. It holds 64 KB
// of the stream: a segment's bytes are read() a piece at a time, and zero
// bytes are counted until a nonzero one says whose they are, so nothing is
// held whole, however long (and it keeps the CRC-32 of the stream).
struct AnnexBReader {
  void Init(ByteSource src) {
    src_ = src;
    crc_.Init();
    buf_.Init();
    pos_ = 0;
    run_ = data0_ = zeros_ = eof_zeros_ = 0;
    end_ = eof_ = false;
  }
  void Quit() { buf_.Quit(); }

  // The current segment's next bytes, at most n of them into p (0: its end)
  size_t read(uint8_t* p, size_t n) {
    size_t k = 0;
    while (k < n) {
      if (data0_) {  // zero bytes that a nonzero byte showed to be the segment's
        size_t m = (size_t)Min<uint64_t>(data0_, n - k);
        memset(p + k, 0, m);
        k += m, data0_ -= m;
        continue;
      }
      if (end_) break;
      if (pos_ == buf_.size() && !fill()) {  // the end of the stream
        end_ = eof_ = true;
        eof_zeros_ = run_, run_ = 0;
        break;
      }
      const uint8_t* q = buf_.data();
      if (!q[pos_]) {
        size_t i = pos_;
        while (i < buf_.size() && !q[i]) i++;
        run_ += i - pos_, pos_ = i;
        continue;
      }
      if (q[pos_] == 1 && run_ >= 2) {  // a start code: the end of the segment
        end_ = true;
        zeros_ = run_ - 2, run_ = 0;
        pos_++;
        break;
      }
      if (run_) {
        data0_ = run_, run_ = 0;
        continue;
      }
      size_t m = Min(buf_.size() - pos_, n - k);
      const uint8_t* z = (const uint8_t*)memchr(q + pos_, 0, m);
      if (z) m = (size_t)(z - (q + pos_));
      memcpy(p + k, q + pos_, m);
      k += m, pos_ += m;
    }
    return k;
  }
  // On to the next NAL unit (what is left of the segment skipped): false at
  // the end of the stream; the zero bytes before its start code
  bool next(uint64_t& zeros) {
    uint8_t skip[256];
    while (read(skip, sizeof skip)) {
    }
    if (eof_) return false;
    zeros = zeros_;
    end_ = false;
    return true;
  }
  // the zero bytes after the last segment (once next() is false)
  uint64_t eof_zeros() const { return eof_zeros_; }
  // the CRC-32 of the stream read so far (all of it, once next() is false)
  uint32_t crc() const { return crc_.value(); }

  // more of the stream in buf_, in place of what was read: false at its end
  bool fill() {
    buf_.resize(1 << 16);
    size_t n = src_.read(buf_.data(), 1 << 16);
    buf_.resize(n);
    crc_.add(buf_);
    pos_ = 0;
    return n != 0;
  }

  ByteSource src_;
  Crc32 crc_;
  Buf<uint8_t> buf_;
  size_t pos_;      // the next byte in buf_
  uint64_t run_;    // zero bytes before it, whose they are not known yet
  uint64_t data0_;  // zero bytes the segment has, not read yet
  uint64_t zeros_, eof_zeros_;
  bool end_;  // the segment has ended
  bool eof_;  // and the stream
};

}  // namespace h264p
