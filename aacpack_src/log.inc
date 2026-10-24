// The values of a unit as a list, in the order the walk reaches them (as
// h264p's log.inc): the compressor parses a unit into it, writes the unit
// back from it to see that it comes out the same, and then codes the values
// from it. The list (ValueLog) is the compressor's, and its room stays for
// the next unit; the channel on it (LogChan), the walk's while it goes.

namespace aacpack {

struct ValueLog {
  Buf<int64_t> v;  // the values
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
  int64_t get() {
    if (UNLIKELY(pos >= v.size())) err_raise_s("internal: more values asked for than there are");
    return v[pos++];
  }
};

// (a channel is made where a walk is to go, Init() telling it its log and
// its direction: parsing, the values go on the end of the log; writing,
// they come from it in turn)
struct LogChan : Chan {
  ValueLog* l_;

  void Init(ValueLog& log, bool w) {
    W = w;
    l_ = &log;
  }
  bool at_end() const { return l_->at_end(); }

  INLINE int64_t io(int64_t x) {
    if (!W) {
      l_->v.push_back(x);
      return x;
    }
    return l_->get();
  }
  int64_t val(int, int, int64_t v, int) override { return io(v); }
  int hv(int, const Huff&, int v, int) override { return (int)io(v); }
  void spec(int, int, int, int* c, int dim) override {
    for (int j = 0; j < dim; j++) c[j] = (int)io(c[j]);
  }
  void bits(int, uint8_t* b, size_t nbits) override {
    size_t n = (nbits + 7) >> 3;
    for (size_t k = 0; k < n; k++) b[k] = (uint8_t)io(b[k]);
  }
  size_t mark() override { return l_->v.size(); }
  void rollback(size_t m) override {
    if (m < l_->v.size()) l_->v.n = m;
  }
};

}  // namespace aacpack
