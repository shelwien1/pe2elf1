// reader.inc -- the input of a run (io.inc's CoIo) cut into chunks at start codes.

// The input cut into chunks at start codes, read into buf: grown as a chunk needs, to IBUF_SIZE
// bytes (and GB_PAD of padding after them). A longer chunk comes in pieces, and so does one that
// would need more memory than there is.
typedef struct Reader {
  CoIo* io;
  U8* buf;
  size_t room;     // buf's size
  size_t cap;      // what it holds of the stream, less the padding
  size_t lo, hi;   // buf[lo, hi): data not returned yet
  int eof, cont;   // cont: the last chunk didn't fit and continues
  U8* pad;         // the GB_PAD bytes after the last chunk, zeroed while it is parsed
  U8 saved[GB_PAD];
} Reader;
// a chunk as rd_chunk() returns it: whole, or a piece of a longer one, the first or a later one
enum { PC_WHOLE, PC_FIRST, PC_MORE };

static int rd_init(Reader* r, CoIo* io) {  // 0: no memory
  memset(r, 0, sizeof(*r));
  r->io = io;
  r->cap = IREAD < IBUF_SIZE ? IREAD : IBUF_SIZE;
  return grow(&r->buf, &r->room, r->cap + GB_PAD, r->cap + GB_PAD);
}

static int rd_fill(Reader* r) {  // moves the data not returned yet to the buffer start and reads more
  size_t n, want;
  if (r->lo) { memmove(r->buf, r->buf + r->lo, r->hi - r->lo); r->hi -= r->lo; r->lo = 0; }
  if (r->eof) return 0;
  if (r->hi == r->cap && r->cap < IBUF_SIZE) {  // full: twice the room, if there is the memory
    const size_t c = r->cap < IBUF_SIZE / 2 ? 2 * r->cap : IBUF_SIZE;
    if (grow(&r->buf, &r->room, c + GB_PAD)) r->cap = c;
  }
  if (r->hi == r->cap) return 0;
  want = r->cap - r->hi < IREAD ? r->cap - r->hi : IREAD;
  n = r->io->read(r->buf + r->hi, want);
  r->hi += n;
  if (n < want) r->eof = 1;
  return n != 0;
}

static int find_sc(const U8* b, size_t* pi, size_t end) {  // 00 00 01 in b[*pi, end); else *pi = where to resume
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

// Next chunk: from a start code (or the stream start) up to the next start code, in
// buf[*pos, *pos + *len), followed by GB_PAD zero bytes until the next call. *part: PC_WHOLE, or
// PC_FIRST, the first piece of a chunk longer than the buffer, PC_MORE a later piece. 0 at the end
// of the input.
static int rd_chunk(Reader* r, size_t* pos, size_t* len, int* part) {
  size_t i;
  int found;
  if (r->pad) { memcpy(r->pad, r->saved, GB_PAD); r->pad = 0; }
  if (r->lo == r->hi && !rd_fill(r)) return 0;
  *part = r->cont ? PC_MORE : PC_WHOLE;
  for (i = r->lo + !r->cont;;) {
    if ((found = find_sc(r->buf, &i, r->hi))) {
      if (i == r->lo) { *part = PC_WHOLE; r->cont = 0; i++; continue; }   // a continued chunk ended at the piece boundary
      r->cont = 0;
      break;
    }
    i -= r->lo;
    if (!rd_fill(r)) {
      if (r->eof) { i = r->hi; r->cont = 0; break; }
      r->cont = 1;                      // full buffer: return a piece, keep a possible start code prefix
      if (*part == PC_WHOLE) *part = PC_FIRST;
      break;
    }
    i += r->lo;
  }
  *pos = r->lo;
  *len = i - r->lo;
  r->lo = i;
  r->pad = r->buf + i;
  memcpy(r->saved, r->pad, GB_PAD);
  memset(r->pad, 0, GB_PAD);
  return 1;
}
