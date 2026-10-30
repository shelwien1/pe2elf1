// reader.inc -- the input of a run (io.inc's CoIo) cut into chunks at start codes.

// a chunk as Reader::chunk() returns it: whole, or a piece of a longer one, the first or a later one
enum { PC_WHOLE, PC_FIRST, PC_MORE };

// 00 00 01 in b[*pi, end); else *pi = where to resume
static int find_sc(const U8* b, size_t* pi, size_t end) {
  size_t i = *pi;
  while (i + SC_PREFIX <= end) {  // (b[i + 2]: the prefix's last byte, if it starts at i)
    if (b[i + 2] > 1) i += SC_PREFIX;
    else if (!b[i + 2]) i++;
    else if (!b[i] && !b[i + 1]) { *pi = i; return 1; }
    else i += SC_PREFIX;
  }
  *pi = i;
  return 0;
}

// The input cut into chunks at start codes, read into buf: grown as a chunk needs, to IBUF_SIZE
// bytes (and GB_PAD of padding after them). A longer chunk comes in pieces, and so does one that
// would need more memory than there is.
struct Reader {
  CoIo* io;
  U8* buf;
  size_t room;     // buf's size
  size_t cap;      // what it holds of the stream, less the padding
  size_t lo, hi;   // buf[lo, hi): data not returned yet
  int eof, cont;   // cont: the last chunk didn't fit and continues
  U8* pad;         // the GB_PAD bytes after the last chunk, zeroed while it is parsed
  U8 saved[GB_PAD];

  int init(CoIo* from) {  // 0: no memory
    memset(this, 0, sizeof *this);
    io = from;
    cap = IREAD < IBUF_SIZE ? IREAD : IBUF_SIZE;
    return grow(&buf, &room, cap + GB_PAD, cap + GB_PAD);
  }
  void quit() {
    free(buf);
    buf = 0;
  }

  int fill() {  // moves the data not returned yet to the buffer start and reads more
    size_t n, want;
    if (lo) { memmove(buf, buf + lo, hi - lo); hi -= lo; lo = 0; }
    if (eof) return 0;
    if (hi == cap && cap < IBUF_SIZE) {  // full: twice the room, if there is the memory
      const size_t c = cap < IBUF_SIZE / 2 ? 2 * cap : IBUF_SIZE;
      if (grow(&buf, &room, c + GB_PAD)) cap = c;
    }
    if (hi == cap) return 0;
    want = cap - hi < IREAD ? cap - hi : IREAD;
    n = io->read(buf + hi, want);
    hi += n;
    if (n < want) eof = 1;
    return n != 0;
  }

  // Next chunk: from a start code (or the stream start) up to the next start code, in
  // buf[*pos, *pos + *len), followed by GB_PAD zero bytes until the next call. *part: PC_WHOLE, or
  // PC_FIRST, the first piece of a chunk longer than the buffer, PC_MORE a later piece. 0 at the end
  // of the input.
  int chunk(size_t* pos, size_t* len, int* part) {
    size_t i;
    int found;
    if (pad) { memcpy(pad, saved, GB_PAD); pad = 0; }
    if (lo == hi && !fill()) return 0;
    *part = cont ? PC_MORE : PC_WHOLE;
    for (i = lo + !cont;;) {
      if ((found = find_sc(buf, &i, hi))) {
        if (i == lo) { *part = PC_WHOLE; cont = 0; i++; continue; }   // a continued chunk ended at the piece boundary
        cont = 0;
        break;
      }
      i -= lo;
      if (!fill()) {
        if (eof) { i = hi; cont = 0; break; }
        cont = 1;                      // full buffer: return a piece, keep a possible start code prefix
        if (*part == PC_WHOLE) *part = PC_FIRST;
        break;
      }
      i += lo;
    }
    *pos = lo;
    *len = i - lo;
    lo = i;
    pad = buf + i;
    memcpy(saved, pad, GB_PAD);
    memset(pad, 0, GB_PAD);
    return 1;
  }
};
