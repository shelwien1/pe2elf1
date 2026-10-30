// io.inc -- the coroutines' input and output as runs of bytes, the input cut into chunks at
// start codes, and the CRC-32.

//--- streams: coroutines -----------------------------------------------------------------------

// m4vpack's compressor and decompressor are coroutines of Lib3 (Lib3/coro3b.inc), in its buffer
// protocol: the input comes in the buffers addinp() gives, a 0-length one with f_quit set at its
// end, and yield Y_IN asks for the next; the output goes into the buffer addout() gave, yield
// Y_OUT handing it over full; Y_END ends a run (its last output in the buffer), Y_FAIL after an
// error (the run's note says it). Y_PART is MTwrap's: a worker's part is done (threads, below).
enum { Y_END = 0, Y_IN = 1, Y_OUT = 2, Y_FAIL = 3, Y_PART = 4 };

constexpr int NOTE_MAX = 256;  // an error's note (a line)
constexpr U64 OVER_MAX = 16;   // bytes the range decoder may ask for past the input's end; more: damaged data

// A coroutine's input and output as runs of bytes. Plain data: io_init() sets it up as the run
// starts (MTwrap's workers are never constructed).
struct CoIo : Coroutine {
  int eof;          // the input has ended
  U64 n_in, n_out;  // the bytes read and written
  U64 over;         // bytes asked for past the end of the input (getb(): the range decoder's)
  int code;         // the exit status an error stands for (E_DATA, E_INTERNAL)
  char note[NOTE_MAX];  // the error, a line

  void io_init(void) {
    eof = 0;
    n_in = n_out = over = 0;
    code = 0;
    note[0] = 0;
  }
  // an error (exit status c): its note, and c to return
  int fail(int c, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(note, sizeof note, fmt, ap);
    va_end(ap);
    code = c;
    return c;
  }
  // the next input buffer (0: the input has ended)
  int next_input(void) {
    if (eof) return 0;
    chkinp();
    if (inpptr >= inpend) eof = 1;  // a 0-length buffer: the end
    return !eof;
  }
  // up to n bytes of the input, fewer only at its end
  size_t read(U8* p, size_t n) {
    size_t k = 0;
    while (k < n) {
      size_t l;
      if (inpptr >= inpend && !next_input()) break;
      l = n - k < (size_t)(inpend - inpptr) ? n - k : (size_t)(inpend - inpptr);
      memcpy(p + k, inpptr, l);
      inpptr += l;
      k += l;
    }
    n_in += k;
    return k;
  }
  // all n bytes into the output
  void write(const U8* p, size_t n) {
    while (n) {
      size_t l;
      if (outptr >= outend) chkout();
      l = n < (size_t)(outend - outptr) ? n : (size_t)(outend - outptr);
      memcpy(outptr, p, l);
      outptr += l;
      p += l;
      n -= l;
      n_out += l;
    }
  }
  INLINE int getb(void) {  // a byte of the input, -1 after its end
    if (inpptr >= inpend && !next_input()) {
      over++;
      return -1;
    }
    n_in++;
    return *inpptr++;
  }
  INLINE void putb(int c) {
    if (outptr >= outend) chkout();
    *outptr++ = (U8)c;
    n_out++;
  }
};

// The input cut into chunks at start codes, read into buf: grown as a chunk needs, to IBUF_SIZE
// bytes (and GB_PAD of padding after them). A longer chunk comes in pieces, and so does one that
// would need more memory than there is.
typedef struct Reader {
  CoIo* io;
  U8* buf;
  size_t cap;      // buf's size, less the padding
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
  r->buf = (U8*)malloc(r->cap + GB_PAD);
  return r->buf != 0;
}

static int rd_fill(Reader* r) {  // moves the data not returned yet to the buffer start and reads more
  size_t n, want;
  if (r->lo) { memmove(r->buf, r->buf + r->lo, r->hi - r->lo); r->hi -= r->lo; r->lo = 0; }
  if (r->eof) return 0;
  if (r->hi == r->cap && r->cap < IBUF_SIZE) {  // full: twice the room, if there is the memory
    const size_t c = r->cap < IBUF_SIZE / 2 ? 2 * r->cap : IBUF_SIZE;
    U8* b = (U8*)realloc(r->buf, c + GB_PAD);
    if (b) r->buf = b, r->cap = c;
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

//--- CRC-32 -------------------------------------------------------------------------------------

constexpr U32 CRC_POLY = 0xEDB88320u;  // (reflected)
static U32 crc_tab[BYTE_VALUES];
static void crc_init(void) {
  U32 i, k, v;
  for (i = 0; i < BYTE_VALUES; i++) {
    for (v = i, k = 0; k < BYTE_BITS; k++) v = (v >> 1) ^ (CRC_POLY & (0u - (v & 1)));
    crc_tab[i] = v;
  }
}
static U32 crc32_upd(U32 crc, const U8* p, size_t n) {
  crc = ~crc;
  while (n--) crc = crc_tab[(crc ^ *p++) & BYTE_MASK] ^ (crc >> BYTE_BITS);
  return ~crc;
}

