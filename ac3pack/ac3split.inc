// ac3split.inc -- mt.inc's Ac3Split: c -t's splitter, the stream in, its parts out.

// The splitter: the stream in, its parts out as frames for MTwrap's parts mode (u64 L, then the
// part's L bytes), on the main thread. It finds the frames as the reader does (reader.inc's
// frame_at()). Plain data (a part of MtEnc, mapped by coro_new()): Init() sets it up, Quit() frees
// what it holds.
struct Ac3Split : Coroutine {
  U64 chunksize, cap;  // -c's part size; mt_cap() of it
  U8* buf;             // the stream from begin on: the part under way
  size_t len, room;
  U64 begin, scan;   // the part's start; where to look for a frame next (stream offsets)
  U64 total, parts;  // the stream's size (at its end), the parts
  int eof;

  void Init(U64 part) {
    coro_init();
    chunksize = part;
    cap = mt_cap(part);
    buf = 0;
    len = room = 0;
    begin = scan = total = parts = 0;
    eof = 0;
  }
  void Quit(void) {
    free(buf);
    buf = 0;
    len = room = 0;
  }

  void put(pcU8 p, size_t n) {
    while (n) {
      uint l;
      chkout();
      l = (uint)(n < getoutleft() ? n : getoutleft());
      memcpy(outptr, p, l);
      outptr += l;
      p += l;
      n -= l;
    }
  }

  // the part under way, up to the stream offset x, as a frame; the next starts at x
  void emit(U64 x) {
    const size_t n = (size_t)(x - begin);
    U8 h[MT_FRAME];
    put_le(h + MT_F_LEN, (U64)n, U64_BYTES);
    put(h, MT_FRAME);
    put(buf, n);
    memmove(buf, buf + n, len - n);
    len -= n;
    begin = x;
    parts++;
  }

  void do_process(void) {
    for (;;) {
      uint l;
      chkinp();
      l = getinplen();
      if (!l) break;  // the end
      if (!grow(&buf, &room, len + l, IO_BUF)) no_memory();
      memcpy(buf + len, inpptr, l);
      inpptr += l;
      len += l;
      scan_input();
    }
    eof = 1;
    scan_input();
    total = begin + len;
    if (total > begin) emit(total);
    yield(this, Y_END);
  }
  // the frames in what came of the input, but one whose next bytes are to come: a cut before a
  // frame where one is due, and where a stretch without frames reaches past twice the cap, there
  void scan_input(void) {
    for (;;) {
      size_t i = (size_t)(scan - begin);
      int n = 0;
      if (i + RD_AHEAD > len && !eof) return;  // (what a frame at i needs, to come)
      if (i >= len) return;
      if (buf[i] == 0x0B && i + 1 < len && buf[i + 1] == 0x77) n = frame_at(buf + i, len - i, eof);
      if (n) {
        // the first substream's frame (AC-3, or E-AC-3's independent substream 0)
        const int first = (buf[i + 5] >> 3) <= 10 || (buf[i + 2] >> 3) == 0;
        if (scan > begin && ((first && scan - begin >= chunksize) || scan - begin >= cap)) emit(scan);
        scan += (U64)n;
      } else {
        scan++;
      }
      if (scan - begin >= mt_hard(cap)) emit(scan);
    }
  }
};
