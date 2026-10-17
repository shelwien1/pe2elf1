// The values of a NAL unit as a list, in the order the walk reaches them: the
// compressor parses a NAL unit into it, writes the NAL unit back from it to
// see that it comes out the same, and then codes the values from it.

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

  int64_t io(int64_t x) {
    if (!W) {
      v_.push_back(x);
      return x;
    }
    if (pos_ >= v_.size()) fail("more values asked for than there are");
    return v_[pos_++];
  }
  int io32(int x) { return (int)io(x); }

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
  void block(int, int, int* c, int n, bool) override {
    for (int i = 0; i < n; i++) c[i] = io32(c[i]);
  }
  void pcm(int& align, int, int* s, int n) override {
    align = io32(align);
    for (int i = 0; i < n; i++) s[i] = io32(s[i]);
  }
  [[noreturn]] void fail(const std::string& msg) override { throw TsvError(msg); }

  std::vector<int64_t> v_;
  size_t pos_ = 0;
};

}  // namespace h264p
