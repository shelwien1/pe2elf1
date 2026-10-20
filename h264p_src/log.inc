// The values of a NAL unit as a list, in the order the walk reaches them: the
// compressor parses a NAL unit into it, writes the NAL unit back from it to
// see that it comes out the same (a slice: the model pass sees to that), and
// then codes the values from it. (Its buffer only grows, for the next NAL
// unit.)

namespace h264p {

struct LogChan : Chan {
  LogChan() : Chan(false) {}

  void clear() {
    n_ = 0;
    pos_ = 0;
    W = false;
  }
  // from the start, for writing
  void rewind() {
    pos_ = 0;
    W = true;
  }
  bool at_end() const { return pos_ == n_; }

  int64_t io(int64_t x) {
    if (!W) {
      if (n_ == v_.size()) grow(1);
      v_[n_++] = x;
      return x;
    }
    if (pos_ >= n_) fail("more values asked for than there are");
    return v_[pos_++];
  }
  int io32(int x) { return (int)io(x); }
  // n values at once
  void io32(int* c, int n) {
    if (!W) {
      if (n_ + n > v_.size()) grow(n);
      int64_t* p = v_.data() + n_;
      for (int i = 0; i < n; i++) p[i] = c[i];
      n_ += n;
      return;
    }
    if (n > 0 && pos_ + n > n_) fail("more values asked for than there are");
    const int64_t* p = v_.data() + pos_;
    for (int i = 0; i < n; i++) c[i] = (int)p[i];
    pos_ += n;
  }
  void grow(size_t n) { v_.resize(std::max(2 * v_.size(), n_ + n + 4096)); }

  void hbegin(const char*) override {}
  int64_t hval(int64_t v) override { return io(v); }
  void hbytes(std::vector<uint8_t>& b) override {
    for (auto& x : b) x = (uint8_t)io(x);
  }
  void hend() override {}
  bool hmore(const char*, bool m) override { return io(m) != 0; }
  int skip(int v) override { return io32(v); }
  int end(int v, bool) override { return io32(v); }
  void mb_begin() override {}
  void mb_end() override {}
  int field(int v) override { return io32(v); }
  int mb_type(int v) override { return io32(v); }
  int t8x8(int v) override { return io32(v); }
  int ipm(int, int v) override { return io32(v); }
  int cpred(int v) override { return io32(v); }
  int sub(int, int v) override { return io32(v); }
  int ref(int, int, int, int, int, int v) override { return io32(v); }
  int mvd(int, int, int, int, int, int, int v) override { return io32(v); }
  int cbp(int v) override { return io32(v); }
  int qpd(int v) override { return io32(v); }
  void block(int, int, int* c, int n, bool) override { io32(c, n); }
  void pcm(int& align, int& nb, int* s, int n) override {
    nb = io32(nb);
    align = io32(align);
    io32(s, n);
  }
  [[noreturn]] void fail(const std::string& msg) override { throw TsvError(msg); }

  std::vector<int64_t> v_;  // the values: n_ of them
  size_t n_ = 0, pos_ = 0;
};

}  // namespace h264p
