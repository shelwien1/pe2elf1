// The byte streams a compression or a decompression runs between: those of
// the coroutine the codec is. The model derives from the range coder (with
// its statistics, fam.inc's tc_rc), the coder from RcIo here (RC_IO_BASE of
// rc.inc), and RcIo from CoroIo and that from Lib3's Coroutine, whose buffer
// protocol they follow: the input comes in the buffers addinp() gives, a
// 0-length one with f_quit set at its end, and yield 1 asks for the next;
// the output goes into the buffer addout() gave, yield 2 handing it over
// full. The coder's bytes go the same way: coding, into the output buffer;
// decoding, from the input buffer -- and past its end the decoder gets 0xFF,
// which is what the coder's flush leaves off; a few of those end every
// stream, many more mean the stream was cut short.

namespace h264p {

// Where bytes come from: a coroutine's input (CoroIo), or a file. read()
// gives up to n bytes, fewer only at the end of the input (0 there).
// (Plain function pointers: a coroutine is plain data.)
struct ByteSource {
  size_t (*fn)(void* ctx, uint8_t* p, size_t n);  // (none: no bytes)
  void* ctx;
  size_t read(uint8_t* p, size_t n) const { return fn ? fn(ctx, p, n) : 0; }
};

// A coroutine's input and output as runs of bytes. Plain data, which
// io_init() sets up as do_process() starts: MTwrap's workers (mt.inc) are
// never constructed. A yield copies the coroutine's stack out into
// Coroutine::stk (STKPAD bytes), so one that would need more fails the run
// instead (before_yield()).
struct CoroIo : Coroutine {
  bool eof;              // the input has ended
  uint64_t n_in, n_out;  // the bytes read and written
  size_t depth;          // the stack a yield copied out, at most

  void io_init() {
    eof = false;
    n_in = n_out = 0;
    depth = 0;
  }

  // up to n bytes of the input, fewer only at its end (yield 1 for more)
  size_t read(uint8_t* p, size_t n) {
    size_t k = 0;
    while (k < n) {
      if (inpptr >= inpend && !next_input()) break;
      size_t l = Min(n - k, (size_t)(inpend - inpptr));
      memcpy(p + k, inpptr, l);
      inpptr += l;
      k += l;
    }
    n_in += k;
    return k;
  }

  // all n bytes into the output (yield 2 with each full buffer)
  void write(const uint8_t* p, size_t n) {
    while (n) {
      if (outptr >= outend) next_output();
      size_t l = Min(n, (size_t)(outend - outptr));
      memcpy(outptr, p, l);
      outptr += l;
      p += l;
      n -= l;
      n_out += l;
    }
  }

  // the next input buffer (false: the input has ended), the next output
  // buffer: a yield for either (wait()), the coroutine's traps not jumped
  // to while it waits
  NOINLINE void wait(bool in) {
    before_yield();
    ErrTrap* t = err_hold();
    if (in)
      chkinp();
    else
      chkout();
    err_resume(t);
  }
  bool next_input() {
    if (eof) return false;
    wait(true);
    if (inpptr >= inpend) eof = true;  // a 0-length input: the end
    return !eof;
  }
  void next_output() { wait(false); }

  // the stack a yield is about to copy out, against Coroutine::stk's room
  void before_yield() {
    char here;
    size_t d = (size_t)((const char*)stkptrH - &here);
    if (d > depth) depth = d;
    if (d + 4096 > (size_t)STKPAD) err_raise("internal: the coroutine's stack (%zu bytes) is too deep to yield", d);
  }

  static size_t read_cb(void* c, uint8_t* p, size_t n) { return ((CoroIo*)c)->read(p, n); }
  // the input as a ByteSource (the compressor's AnnexBReader reads it)
  ByteSource source() {
    ByteSource s;
    s.fn = read_cb;
    s.ctx = this;
    return s;
  }
};

// and the range coder's bytes, one at a time
template <class RC>
struct RcIo : CoroIo {
  int over;  // decoding: the bytes taken past the end of the input

  void io_init() {
    CoroIo::io_init();
    over = 0;
  }
  INLINE void put(uint8_t c) {
    if (UNLIKELY(outptr >= outend)) next_output();
    *outptr++ = c;
    n_out++;
  }
  INLINE uint8_t get() {
    if (UNLIKELY(inpptr >= inpend) && !next_input()) {
      if (++over > 48) err_raise("the compressed stream is cut short");
      return 0xFF;
    }
    n_in++;
    return *inpptr++;
  }
};

}  // namespace h264p

#define RC_IO_BASE h264p::RcIo
