// The range coder's byte I/O (RC_IO_BASE of rc.inc): coding, the bytes
// collect in a buffer the caller writes out; decoding, they are read from a
// file. Past the end of the file the decoder gets 0xFF, which is what the
// coder's flush leaves off; a few of those end every stream, many more mean
// the stream was cut short.

namespace h264p {

template <class RC>
struct RcIo {
  FILE* f = nullptr;         // decoding: the compressed stream
  std::vector<uint8_t> buf;  // coding: the bytes so far; decoding: what was read
  size_t pos = 0;
  int over = 0;

  void put(uint8_t c) { buf.push_back(c); }
  uint8_t get() {
    if (pos == buf.size()) {
      buf.resize(1 << 16);
      size_t n = f ? fread(buf.data(), 1, buf.size(), f) : 0;
      buf.resize(n);
      pos = 0;
      if (!n) {
        if (++over > 48) cm_fail("the compressed stream is cut short");
        return 0xFF;
      }
    }
    return buf[pos++];
  }
};

}  // namespace h264p

#define RC_IO_BASE h264p::RcIo
