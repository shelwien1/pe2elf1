// The byte streams a compression or a decompression runs between, and the
// range coder's byte I/O (RC_IO_BASE of rc.inc): coding, the bytes collect
// in a buffer the caller writes out; decoding, they are read from a
// ByteSource. Past the end of the input the decoder gets 0xFF, which is
// what the coder's flush leaves off; a few of those end every stream, many
// more mean the stream was cut short.

namespace h264p {

// Where bytes come from and go to: the coroutine of coro.inc (yielding for
// more input, or for a free output buffer), or a file. read() gives up to n
// bytes, fewer only at the end of the input (0 there); write() takes all n.
// (Plain function pointers: the coroutine is plain data, never constructed.)
struct ByteSource {
  size_t (*fn)(void* ctx, uint8_t* p, size_t n) = nullptr;
  void* ctx = nullptr;
  size_t read(uint8_t* p, size_t n) const { return fn ? fn(ctx, p, n) : 0; }
};
struct ByteSink {
  void (*fn)(void* ctx, const uint8_t* p, size_t n) = nullptr;
  void* ctx = nullptr;
  void write(const uint8_t* p, size_t n) const {
    if (n) fn(ctx, p, n);
  }
};

template <class RC>
struct RcIo {
  ByteSource src;            // decoding: the compressed stream
  std::vector<uint8_t> buf;  // coding: the bytes so far; decoding: what was read
  size_t pos = 0;
  int over = 0;

  void put(uint8_t c) { buf.push_back(c); }
  uint8_t get() {
    if (pos == buf.size()) {
      buf.resize(1 << 16);
      size_t n = src.read(buf.data(), buf.size());
      buf.resize(n);
      pos = 0;
      if (!n) {
        if (++over > 48) cm_fail("the compressed stream is cut short");
        return 0xFF;
      }
    }
    return buf[pos++];
  }
  // decoding from src, from its start
  void reset(ByteSource s) {
    src = s;
    buf.clear();
    pos = 0;
    over = 0;
  }
};

}  // namespace h264p

#define RC_IO_BASE h264p::RcIo
