// coio.inc -- io.inc's CoIo: a coroutine's input and output as runs of bytes.

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
