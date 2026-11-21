// reader.inc -- the input of a run (io.inc's CoIo) cut into core frames and the bytes between them.

// what Reader::next() returns: the end of the input, a frame, bytes that aren't a frame
enum { RD_END, RD_FRAME, RD_RAW };

// the bytes a frame at i needs after it to be taken for one: itself, and the next one's header
// (packed 14 bits a word, pack.inc's: 8/7 as many)
constexpr size_t RD_AHEAD = MAX_FRAME * 8 / 7 + 64;

// The size of the core frame at p (avail bytes there, eof: they are the last of the input), where
// it is taken for one: a sync word and a header whose frame size and block count are in range, and
// where another follows it, the input ends after it, or it parses (into scratch); 0 where it isn't
static INLINE int frame_at(pcU8 p, size_t avail, int eof, CoreRec* scratch) {
  if (avail < 4 || p[0] != 0x7F || p[1] != 0xFE) return 0;  // (most bytes don't start a sync word)
  const int n = core_size(p, avail);
  if (!n) return 0;
  if (((size_t)n == avail && eof) || core_size(p + n, avail - n)) return n;
  return core_parse(p, n, *scratch) ? 0 : n;
}
// The same of a frame packed by pk (pack.inc), unpacked into ub (MAX_FRAME + PAD bytes: *ul of
// them): its packed bytes, where another frame packed so follows it, the input ends after it, or it
// parses; 0 where it isn't one. *ex: whether packing those bytes again makes these (where not, it
// is kept as bytes, its blocks counted as a frame's). (PK_BE16: frame_at()'s, ub left as it was.)
static int frame_at_pk(int pk, pcU8 p, size_t avail, int eof, CoreRec* scratch, U8* ub, int* ul, int* ex) {
  *ex = 1;
  if (pk == PK_BE16) return *ul = frame_at(p, avail, eof, scratch);
  if (pack_sync(p, avail) != pk) return 0;
  const size_t hold = pack_holds(pk, avail);
  U8 h[16];
  if (hold < 16) return 0;
  pack_unpack(pk, p, h, 16);
  const int n = core_size(h, hold < MAX_FRAME ? hold : MAX_FRAME);
  if (!n) return 0;
  const size_t cl = pack_size(pk, (size_t)n);
  pack_unpack(pk, p, ub, (size_t)n);
  memset(ub + n, 0, PAD);
  bool ok = cl == avail && eof;
  if (!ok && pack_sync(p + cl, avail - cl) == pk && pack_holds(pk, avail - cl) >= 16) {
    pack_unpack(pk, p + cl, h, 16);
    ok = core_size(h, pack_holds(pk, avail - cl)) != 0;
  }
  if (!ok && core_parse(ub, n, *scratch)) return 0;
  *ex = pack_same(pk, ub, (size_t)n, p);
  *ul = n;
  return (int)cl;
}
// the packing of the first frame in the n bytes at p (eof: all of the input), as frame_at_pk()
// takes one: PK_BE16 where there is none
static int pack_first(pcU8 p, size_t n, int eof, CoreRec* scratch) {
  static U8 ub[MAX_FRAME + PAD];  // (the main thread's)
  int ul, ex;
  for (size_t i = 0; i + 4 <= n; i++) {
    const int pk = pack_sync(p + i, n - i);
    if (pk >= 0 && frame_at_pk(pk, p + i, n - i, eof, scratch, ub, &ul, &ex)) return pk;
  }
  return PK_BE16;
}
// the frames packed by pk in the n bytes at p (eof: all of the input) unpacked into a new buffer
// (*m bytes, PAD zero bytes after them), the bytes between them left out: what c's search for the
// binding reads of a packed stream
static U8* pack_frames(int pk, pcU8 p, size_t n, int eof, size_t* m, CoreRec* scratch) {
  U8* out = (U8*)xmalloc(n + PAD);  // (unpacked they are no more)
  size_t o = 0;
  int ul, ex;
  for (size_t i = 0; i < n;) {
    const int c = frame_at_pk(pk, p + i, n - i, eof, scratch, out + o, &ul, &ex);
    if (!c) {
      i++;
      continue;
    }
    o += (size_t)ul, i += (size_t)c;
  }
  memset(out + o, 0, PAD);
  *m = o;
  return out;
}

// The input cut into frames and the bytes between them, read into buf (from io, CoIo's: what
// next() takes): grown as the input needs, to IBUF_SIZE bytes. A run of bytes longer than the
// buffer holds comes in pieces. (ac3pack's, for core frames.)
struct Reader {
  U8* buf;
  size_t room;    // buf's size
  size_t cap;     // what it holds of the input at most
  size_t lo, hi;  // buf[lo, hi): read and not returned yet
  int eof;
  CoreRec* scratch;  // where a frame is parsed to tell it
  int pk;            // the stream's packing (pack.inc; set after init())
  U8 ub[MAX_FRAME + PAD];  // a frame packed otherwise than PK_BE16, unpacked (ul bytes: next()'s)
  int ul, ux;              // (and whether its packing makes it again)

  int init(CoreRec* s) {  // 0: no memory
    memset(this, 0, sizeof *this);
    scratch = s;
    cap = RD_AHEAD + IREAD < IBUF_SIZE ? RD_AHEAD + IREAD : IBUF_SIZE;
    return grow(&buf, &room, cap + PAD, cap + PAD);
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
      if (grow(&buf, &room, c + PAD)) cap = c;
    }
    if (hi == cap) return 0;
    want = cap - hi < IREAD ? cap - hi : IREAD;
    n = io.read(buf + hi, want);
    hi += n;
    memset(buf + hi, 0, PAD);  // (a frame's bits are read with PAD bytes after them)
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
      const int n = frame_at_pk(pk, buf + i, hi - i, eof, scratch, ub, &ul, &ux);
      if (n) {
        if (i > lo) break;  // the bytes before it first
        *pos = i;
        *len = (size_t)n;
        lo = i + n;
        return RD_FRAME;
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
