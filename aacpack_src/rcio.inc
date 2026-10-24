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

namespace aacpack {

// Where bytes come from: a coroutine's input (CoroIo), or a file. read()
// gives up to n bytes, fewer only at the end of the input (0 there).
// (Plain function pointers: a coroutine is plain data.)
struct ByteSource {
  size_t (*fn)(void* ctx, uint8_t* p, size_t n);  // (none: no bytes)
  void* ctx;
  size_t read(uint8_t* p, size_t n) const { return fn ? fn(ctx, p, n) : 0; }
};

// What a codec's coroutine yields with: the buffer protocol's kYieldIn (it
// needs its next input), kYieldOut (its output buffer is full), kYieldEnd
// (the end) and kYieldFail (an error, said in its note); and what it tells
// its driver, which shows what it will of it (a codec says nothing on its
// own): kYieldAudio, what the stream is (its audio, at the first frame and
// where that changes), kYieldNote, a message (its note), kYieldStats, its
// statistics are ready to be read (at the end), and kYieldTick, a point at
// which the driver may show the progress (each frame). (4 is MTwrap's.)
enum : uint {
  kYieldEnd = 0, kYieldIn = 1, kYieldOut = 2, kYieldFail = 3,
  kYieldAudio = 5, kYieldNote = 6, kYieldStats = 7, kYieldTick = 8
};

// What the stream is, from its frames (cmstream.inc's audio_of()): plain
// values, compared as bytes, all zero where it is not known
struct AudioInfo {
  int container;    // 1 ADTS, 2 LOAS/LATM, 3 ADIF, 4 AUS1, 5 raw access units, 6 MP4
  int aot;          // the core's audio object type
  int sf_index;     // its sampling frequency index
  int chan_config;  // channel configuration (0: by a PCE)
  int sbr, ps;      // SBR, PS seen in its payloads (0 not yet)
  int frame960;

  // a, of a frame, added to what this says: whether this changed
  bool add(const AudioInfo& a) {
    AudioInfo u = a;
    if (!memcmp(this, &a, offsetof(AudioInfo, sbr))) u.sbr |= sbr, u.ps |= ps;
    if (!memcmp(this, &u, sizeof u)) return false;
    *this = u;
    return true;
  }
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
  // what it tells its driver (above): the frames so far (each kYieldTick),
  // a message (kYieldNote, kYieldFail: a line of text, with its '\n'), the
  // audio (kYieldAudio)
  uint64_t frames;
  char note[512];
  AudioInfo audio;

  void io_init() {
    eof = false;
    n_in = n_out = 0;
    depth = 0;
    frames = 0;
    note[0] = 0;
    memset((void*)&audio, 0, sizeof audio);
  }

  // a yield of code to the driver, which tells it something (above): the
  // coroutine's traps put aside while it waits
  NOINLINE void tell(uint code) {
    before_yield();
    ErrTrap* t = err_hold();
    yield(static_cast<Coroutine*>(this), code);
    err_resume(t);
  }
  // the note, a line made of fmt (a '\n' after it): the error the run will
  // end with (kYieldFail), or (tell_note()) a message told now
  void vnote(const char* fmt, va_list ap) {
    vsnprintf(note, sizeof note - 1, fmt, ap);
    size_t n = strlen(note);
    if (!n || note[n - 1] != '\n') note[n] = '\n', note[n + 1] = 0;
  }
  void set_note(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vnote(fmt, ap);
    va_end(ap);
  }
  void tell_note(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vnote(fmt, ap);
    va_end(ap);
    tell(kYieldNote);
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
  // the input as a ByteSource (the compressor's window reads it)
  ByteSource source() {
    ByteSource s;
    s.fn = read_cb;
    s.ctx = this;
    return s;
  }
};

// and the range coder's bytes, one at a time (decoding, every byte the
// coder wrote and no more: rc.inc's flush)
template <class RC>
struct RcIo : CoroIo {
  INLINE void put(uint8_t c) {
    if (UNLIKELY(outptr >= outend)) next_output();
    *outptr++ = c;
    n_out++;
  }
  INLINE uint8_t get() {
    if (UNLIKELY(inpptr >= inpend) && !next_input()) err_raise("the compressed stream is cut short");
    n_in++;
    return *inpptr++;
  }
  // whether the input has ended (decoding, after the last value: nothing
  // may come after it)
  bool input_done() {
    while (inpptr >= inpend)
      if (!next_input()) return true;
    return false;
  }
};

}  // namespace aacpack

#define RC_IO_BASE aacpack::RcIo
