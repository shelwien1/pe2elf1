// coio.inc -- io.inc's CoIo: a coroutine's input and output as runs of bytes.

// A coroutine's input and output as runs of bytes, on B: Lib3's Coroutine, or a coroutine whose range
// coder (coder.inc's Coder, its base) takes bytes from them and puts bytes into them too. Plain data:
// io_init() sets it up before its run (MTwrap's workers are never constructed). The drivers' buffers
// are counted as they give them (addinp(), addout()), so that the bytes taken and put so far, by
// whatever took and put them, are known (in_taken(), out_put()).
template <class B>
struct CoIo : B {
  using B::inpptr;
  using B::inpend;
  using B::outptr;
  using B::outbeg;
  using B::outend;
  using B::chkinp;
  using B::chkout;
  int eof;          // the input has ended
  U64 got, sent;    // the bytes of the input buffers given, of the output buffers handed over
  int code;         // the exit status an error stands for (E_DATA, E_INTERNAL)
  char note[NOTE_MAX];  // the error, a line

  void io_init(void) {  // (before the driver gives the run its buffers)
    eof = 0;
    got = sent = 0;
    code = 0;
    note[0] = 0;
  }
  // the driver's next input buffer (n bytes at p), and its next output buffer (the last handed over)
  void addinp(byte* p, uint n) {
    got += n;
    B::addinp(p, n);
  }
  void addout(byte* p, uint n) {
    sent += (U64)(outptr - outbeg);
    B::addout(p, n);
  }
  // the bytes taken from the input, and put into the output, so far
  U64 in_taken(void) const { return got - (U64)(inpend - inpptr); }
  U64 out_put(void) const { return sent + (U64)(outptr - outbeg); }
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
  size_t read(pU8 p, size_t n) {
    size_t k = 0;
    while (k < n) {
      size_t l;
      if (inpptr >= inpend && !next_input()) break;
      l = n - k < (size_t)(inpend - inpptr) ? n - k : (size_t)(inpend - inpptr);
      memcpy(p + k, inpptr, l);
      inpptr += l;
      k += l;
    }
    return k;
  }
  // all n bytes into the output
  void write(pcU8 p, size_t n) {
    while (n) {
      size_t l;
      if (outptr >= outend) chkout();
      l = n < (size_t)(outend - outptr) ? n : (size_t)(outend - outptr);
      memcpy(outptr, p, l);
      outptr += l;
      p += l;
      n -= l;
    }
  }
};
