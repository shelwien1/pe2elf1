// reader.inc -- the input of a run (io.inc's CoIo) cut into syncframes and the bytes between them.

// what Reader::next() returns: the end of the input, a frame, bytes that aren't a frame
enum { RD_END, RD_FRAME, RD_RAW };

// the bytes a frame at i needs after it to be taken for one: itself, and the next one's header
constexpr size_t RD_AHEAD = MAX_FRAME + 8;

// The size of the frame at p (avail bytes there, eof: they are the last of the input), where it is
// taken for one: a sync word and a header that parses, and where the next frame follows it, the
// input ends after it, or its CRCs hold (as ac3tsv takes them); 0 where it isn't
static int frame_at(pcU8 p, size_t avail, int eof) {
  const int n = frame_bytes(p, avail);
  if (!n || (size_t)n > avail) return 0;
  if (((size_t)n == avail && eof) || frame_bytes(p + n, avail - n)) return n;
  return frame_crc_ok(p, n) ? n : 0;
}

// The input cut into frames and the bytes between them, read into buf (from io, CoIo's: what
// next() takes): grown as the input needs, to IBUF_SIZE bytes. A run of bytes longer than the
// buffer holds comes in pieces.
struct Reader {
  U8* buf;
  size_t room;    // buf's size
  size_t cap;     // what it holds of the input at most
  size_t lo, hi;  // buf[lo, hi): read and not returned yet
  int eof;

  int init() {  // 0: no memory
    memset(this, 0, sizeof *this);
    cap = RD_AHEAD + IREAD < IBUF_SIZE ? RD_AHEAD + IREAD : IBUF_SIZE;
    return grow(&buf, &room, cap, cap);
  }
  void quit() {
    free(buf);
    buf = 0;
  }

  // moves what isn't returned yet to the buffer's start and reads more: 0 where nothing came (the
  // input ended, or the buffer is full and can't grow)
  template <class IO>
  int fill(IO& io) {
    size_t n, want;
    if (lo) {
      memmove(buf, buf + lo, hi - lo);
      hi -= lo;
      lo = 0;
    }
    if (eof) return 0;
    if (hi == cap && cap < IBUF_SIZE) {  // full: twice the room, if there is the memory
      const size_t c = cap < IBUF_SIZE / 2 ? 2 * cap : IBUF_SIZE;
      if (grow(&buf, &room, c)) cap = c;
    }
    if (hi == cap) return 0;
    want = cap - hi < IREAD ? cap - hi : IREAD;
    n = io.read(buf + hi, want);
    hi += n;
    if (n < want) eof = 1;
    return n != 0;
  }

  // The next piece of io: a frame (RD_FRAME) or a run of bytes that aren't one (RD_RAW), in
  // buf[*pos, *pos + *len), until the next call; RD_END at the end of the input
  template <class IO>
  int next(IO& io, rp<size_t> pos, rp<size_t> len) {
    size_t i = lo;
    for (;;) {
      if (hi - i < RD_AHEAD && !eof) {  // (what a frame at i needs, read first)
        const size_t at = i - lo;
        const int got = fill(io);
        i = lo + at;
        if (got || eof) continue;
        if (i > lo) break;  // the buffer full: the bytes before i go now
        fatal("internal error: the reader's buffer can't hold a frame");
      }
      if (i >= hi) break;
      if (buf[i] == 0x0B && i + 1 < hi && buf[i + 1] == 0x77) {
        const int n = frame_at(buf + i, hi - i, eof);
        if (n) {
          if (i > lo) break;  // the bytes before it first
          *pos = i;
          *len = (size_t)n;
          lo = i + n;
          return RD_FRAME;
        }
      }
      i++;
    }
    if (i == lo) return RD_END;
    *pos = lo;
    *len = i - lo;
    lo = i;
    return RD_RAW;
  }
};
