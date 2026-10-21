// The values of a NAL unit as a list, in the order the walk reaches them: the
// compressor parses a NAL unit into it, writes the NAL unit back from it to
// see that it comes out the same (a slice: the model pass sees to that), and
// then codes the values from it. (Its buffer only grows, for the next NAL
// unit.)

namespace h264p {

struct LogChan : Chan {
  LogChan() : Chan(false) {}

  void clear() {
    v_.clear();
    pos_ = 0;
    W = false;
  }
  // from the start, for writing
  void rewind() {
    pos_ = 0;
    W = true;
  }
  bool at_end() const { return pos_ == v_.size(); }

  // a value: 32 bits, the most a value of the walk's has, or a header's
  // 64 in two
  int32_t get() {
    if (pos_ >= v_.size()) fail("more values asked for than there are");
    return v_[pos_++];
  }
  int io32(int x) {
    if (!W) {
      v_.push_back(x);
      return x;
    }
    return get();
  }
  int64_t io(int64_t x) {
    if (!W) {
      v_.push_back((int32_t)x);
      v_.push_back((int32_t)((uint64_t)x >> 32));
      return x;
    }
    uint64_t lo = (uint32_t)get();
    return (int64_t)((uint64_t)(uint32_t)get() << 32 | lo);
  }
  // n values at once; as a block of coefficients (sparse), how many there
  // are up to the last that is not zero, and those
  void io32(int* c, int n) {
    if (!W) {
      v_.insert(v_.end(), c, c + n);
      return;
    }
    if (n > 0 && v_.size() - pos_ < (size_t)n) fail("more values asked for than there are");
    std::copy(v_.data() + pos_, v_.data() + pos_ + n, c);
    pos_ += n;
  }
  void coefs(int* c, int n) {
    int k = n;
    if (!W)
      while (k > 0 && !c[k - 1]) k--;
    k = io32(k);
    if (k < 0 || k > n) fail("more values asked for than there are");
    io32(c, k);
    std::fill(c + k, c + n, 0);
  }

  void hbegin(const char*) override {}
  int64_t hval(int64_t v) override { return io(v); }
  void hbytes(std::vector<uint8_t>& b) override {
    for (auto& x : b) x = (uint8_t)io32(x);
  }
  void hend() override {}
  bool hmore(const char*, bool m) override { return io32(m) != 0; }
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
  void block(int, int, int* c, int n, bool) override { coefs(c, n); }
  void pcm(int& align, int& nb, int* s, int n) override {
    nb = io32(nb);
    align = io32(align);
    io32(s, n);
  }
  [[noreturn]] void fail(const std::string& msg) override { throw TsvError(msg); }

  std::vector<int32_t> v_;  // the values (only grows: its room stays for the next NAL unit)
  size_t pos_ = 0;           // the next one to read
};

}  // namespace h264p
