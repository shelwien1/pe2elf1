// The values of a NAL unit as a list, in the order the walk reaches them: the
// compressor parses a NAL unit into it, writes the NAL unit back from it to
// see that it comes out the same (a slice: the model pass sees to that), and
// then codes the values from it. The list (ValueLog) is the dry run's, and
// its room stays for the next NAL unit; the channel on it (LogChan), the
// walk's while it goes.

namespace h264p {

struct ValueLog {
  Buf<int32_t> v;  // the values
  size_t pos;      // the next one to read

  void Init() {
    v.Init();
    pos = 0;
  }
  void Quit() { v.Quit(); }
  // empty, for a parse; from the start, for writing
  void clear() {
    v.clear();
    pos = 0;
  }
  void rewind() { pos = 0; }
  bool at_end() const { return pos == v.size(); }
};

// (a channel is made where a walk is to go, Init() telling it its log and
// its direction: parsing, the values go on the end of the log; writing,
// they come from it in turn)
struct LogChan : Chan {
  void Init(ValueLog& log, bool w) {
    W = w;
    l_ = &log;
  }
  bool at_end() const { return l_->at_end(); }

  // a value: 32 bits, the most a value of the walk's has, or a header's
  // 64 in two
  int32_t get() {
    if (l_->pos >= l_->v.size()) fail("more values asked for than there are");
    return l_->v[l_->pos++];
  }
  int io32(int x) {
    if (!W) {
      l_->v.push_back(x);
      return x;
    }
    return get();
  }
  int64_t io(int64_t x) {
    if (!W) {
      l_->v.push_back((int32_t)x);
      l_->v.push_back((int32_t)((uint64_t)x >> 32));
      return x;
    }
    uint64_t lo = (uint32_t)get();
    return (int64_t)((uint64_t)(uint32_t)get() << 32 | lo);
  }
  // n values at once; as a block of coefficients (sparse), how many there
  // are up to the last that is not zero, and those
  void io32(int* c, int n) {
    if (!W) {
      l_->v.append((const int32_t*)c, n);
      return;
    }
    if (n > 0 && l_->v.size() - l_->pos < (size_t)n) fail("more values asked for than there are");
    if (n > 0) memcpy(c, l_->v.data() + l_->pos, n * sizeof(int));
    l_->pos += n;
  }
  void coefs(int* c, int n) {
    int k = n;
    if (!W)
      while (k > 0 && !c[k - 1]) k--;
    k = io32(k);
    if (k < 0 || k > n) fail("more values asked for than there are");
    io32(c, k);
    if (k < n) memset(c + k, 0, (n - k) * sizeof(int));
  }

  void hbegin(const char*) override {}
  int64_t hval(int64_t v) override { return io(v); }
  void hbytes(uint8_t* b, size_t n) override {
    for (size_t i = 0; i < n; i++) b[i] = (uint8_t)io32(b[i]);
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
  [[noreturn]] void fail(const char* why) override { err_raise_s(why); }

  ValueLog* l_;
};

}  // namespace h264p
