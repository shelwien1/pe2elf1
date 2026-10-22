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

// What a codec's coroutine yields with: the buffer protocol's kYieldIn (it
// needs its next input), kYieldOut (its output buffer is full), kYieldEnd
// (the end) and kYieldFail (an error, said in its note); and what it tells
// its driver, which shows what it will of it (a codec says nothing on its
// own): kYieldVideo, what the video is (its video, at the first slice and
// where that changes), kYieldNote, a message (its note), kYieldStats, its
// statistics are ready to be read (at the end), and kYieldTick, a point at
// which the driver may show the progress (each picture). (4 is MTwrap's.)
enum : uint {
  kYieldEnd = 0, kYieldIn = 1, kYieldOut = 2, kYieldFail = 3,
  kYieldVideo = 5, kYieldNote = 6, kYieldStats = 7, kYieldTick = 8
};

// What the video is, from its parameter sets and its slices (cmstream.inc's
// video_of()): plain values, compared as bytes, all zero where it is not
// known. Its sequence's (an SPS's), then the tools its slices use, as the
// sets of what they used since the sequence's last changed (bits), so that
// a stream that goes back and forth between them (frame and field
// pictures, PAFF; PPSs of other tools) tells each of them once (add()).
struct VideoInfo {
  int width, height;              // the pictures as shown (the cropping taken off), in samples
  int profile, constraints, level;  // profile_idc, the constraint_set flags, level_idc
  int chroma, depth_luma, depth_chroma;  // chroma_format_idc, bit depths
  int frames_only, mbaff, refs, lossless;  // ..., max_num_ref_frames, transform bypass
  uint32_t num_units_in_tick, time_scale;  // VUI's timing (0: none)
  // the tools: 1 << entropy_coding_mode_flag (1 CAVLC, 2 CABAC), 1 << weighted_bipred_idc
  // where not 0 (2 explicit, 4 implicit), the most slice groups; the rest 1 where used
  int entropy, weighted_b, slice_groups;
  int field, transform_8x8, weighted_p, constrained_intra, scaling;

  // v, of a slice, added to what this says: v where its sequence is not
  // this one's, else this one's tools and v's; whether this changed
  bool add(const VideoInfo& v) {
    VideoInfo u = v;
    if (!memcmp(this, &v, offsetof(VideoInfo, entropy))) {
      u.entropy |= entropy, u.weighted_b |= weighted_b, u.slice_groups = Max(u.slice_groups, slice_groups);
      u.field |= field, u.transform_8x8 |= transform_8x8, u.weighted_p |= weighted_p;
      u.constrained_intra |= constrained_intra, u.scaling |= scaling;
    }
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
  // what it tells its driver (above): the pictures so far (each kYieldTick),
  // a message (kYieldNote, kYieldFail: a line of text, with its '\n'), the
  // video (kYieldVideo)
  uint64_t pictures;
  char note[512];
  VideoInfo video;

  void io_init() {
    eof = false;
    n_in = n_out = 0;
    depth = 0;
    pictures = 0;
    note[0] = 0;
    memset((void*)&video, 0, sizeof video);
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
  // the input as a ByteSource (the compressor's AnnexBReader reads it)
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

}  // namespace h264p

#define RC_IO_BASE h264p::RcIo
