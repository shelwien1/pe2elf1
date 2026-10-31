// ac3coro.inc -- coro.inc's Ac3Coro: a run of c or d, at a level.

// A run of ac3pack: compressing (f_DEC 0) or decompressing (1) a stream, as a coroutine, by the
// model of level LV; the coroutine is the range coder too (coder.inc), whose bytes go through its
// output or come from its input. Plain data, mapped (coro_new(), zeroed) and never constructed:
// Init() sets it up, Quit() frees what it holds. A run shares nothing with another but the constant
// tables, so any number of them can go at once, on as many threads.
template <int f_DEC, class LV>
struct Ac3Coro : CoIo<Coder<f_DEC> > {
  typedef CoIo<Coder<f_DEC> > B;  // (the run's input and output, and its coder)
  using B::coro_init;
  using B::io_init;
  using B::read;
  using B::write;
  using B::fail;
  using B::next_input;
  using B::in_taken;
  using B::out_put;
  using B::inpptr;
  using B::inpend;
  using B::stats;
  using B::orig;
  using B::coder_start;
  using B::coder_end;
  using B::damaged;
  using B::code_u32;
  using B::f_quit;
  static constexpr int f_dec = f_DEC;
  int status;  // the run's: 0, or the exit status of its error (note); 1 until it has ended
  Stats st;    // c's
  U32 crc;     // of the stream: c's input, d's output
  Reader rd;   // c: the input
  Model<f_DEC, LV> k;
  Frame f;                                        // the frame being coded
  U8 fb[MAX_FRAME + PAD], back[MAX_FRAME + PAD];  // c: the frame (and PAD zero bytes); it written back
  U8 pbuf[IO_BUF];                                // d: bytes kept as bytes, a buffer at a time
  // what the progress line shows: the frames so far, and the audio as told (Y_AUDIO)
  U64 nframe;
  AudioInfo audio;
  int is_part;  // d: the stream is a part's (mt.inc's records), which ends where it stops reading

  void Init(int s) {  // (s: c -s, the bits by kind of syntax element: the coder's cost and orig)
    coro_init();
    io_init();
    stats = s;
    status = 1;
    nframe = 0;
    is_part = 0;
    memset((void*)&audio, 0, sizeof audio);
    k.Init();
  }
  // the stream's bytes and the compressed stream's done so far (c: the chunks taken, as they start)
  U64 raw_done(void) const { return f_DEC ? out_put() : (U64)st.in_bytes; }
  U64 packed_done(void) const { return f_DEC ? in_taken() : out_put(); }
  // a frame of the stream's first substream comes as records: where the audio it has is another,
  // the driver is told
  void frame_comes(const Frame& fr) {
    AudioInfo a;
    if (a.set(fr) && !a.same(audio)) {
      audio = a;
      yield(static_cast<Coroutine*>(this), Y_AUDIO);
    }
  }
  void Quit(void) {
    k.Quit();
    rd.quit();
  }

  void do_process(void) {
    int r;
    r = f_DEC ? run_d() : run_c();
    Quit();
    status = r;
    for (;;) yield(static_cast<Coroutine*>(this), r ? Y_FAIL : Y_END);
  }

  // c: the frame (n bytes at fb, PAD zero bytes after them) parsed into f and written back: 0, or
  // why it can't be coded as records (1: it doesn't parse, 2: it doesn't come back the same)
  int parse(int n) {
    NoBlockCb nb;
    BitIn io;
    memset((void*)&f, 0, sizeof f);
    io.init(fb, n * BYTE_BITS);
    io.pos = 16;  // (after the sync word)
    Walk<BitIn, NoBlockCb> w(io, f, nb);
    if (!w.frame() || f.fbytes != n) return 1;
    get_crcs(fb, n, f);
    const int tailbits = f.tailbits;
    BitOut bo;
    bo.init(back, n * BYTE_BITS);
    Walk<BitOut, NoBlockCb> wb(bo, f, nb);
    if (!wb.frame() || bo.pos != n * BYTE_BITS || f.tailbits != tailbits) return 2;
    put_crcs(back, n, f);
    return memcmp(back, fb, (size_t)n) ? 2 : 0;
  }

  int run_c(void) {
    size_t pos, len;
    int t;
    U8 head[AC3P_HEAD];
    memset(&st, 0, sizeof st);
    crc = 0;
    if (!rd.init()) return fail(E_INTERNAL, "out of memory");
    memcpy(head, magic, sizeof magic);
    head[AC3P_AT_VERSION] = AC3P_VERSION;
    head[AC3P_AT_LEVEL] = (U8)LV::id;
    write(head, sizeof head);
    coder_start();
    k.start(this);
    while ((t = rd.next(*this, &pos, &len)) != RD_END) {
      const pU8 p = rd.buf + pos;  // (the chunk, reached through p alone while it is coded)
      int kind = CK_RAW;
      crc = crc32_upd(crc, p, len);
      st.in_bytes += (double)len;
      if (t == RD_FRAME) {
        int why;
        nframe++;
        st.frames++;
        memcpy(fb, p, len);
        memset(fb + len, 0, PAD);
        why = parse((int)len);
        if ((p[5] >> 3) > 10) st.eac3++, st.dep += ((p[2] >> 6) == 1);
        else st.ac3++;
        if (!why) {
          frame_comes(f);
          k.code_kind(CK_FRAME);
          if (!k.frame(f)) return fail(E_INTERNAL, "internal error: a frame parsed couldn't be coded");
          st.packed++;
          st.frame_bytes += (double)len;
          continue;
        }
        if (why == 1) st.raw_parse++;
        else st.raw_verify++;
        kind = CK_BAD;
      } else {
        st.raw_bytes += (double)len;
      }
      orig[CAT_RAW] += (double)BYTE_BITS * len;
      k.code_kind(kind);
      k.code_raw_len(len);
      k.code_raw_bytes(p, len, kind);
    }
    k.code_kind(CK_END);
    code_u32(crc);
    coder_end();
    return 0;
  }

  // d: the data is damaged (said: the stream cut short, where the coder had to read past its end)
  int damaged_data(void) {
    return fail(E_DATA, f_quit ? "damaged data: the stream is cut short" : "damaged data");
  }

  int run_d(void) {
    U8 h[AC3P_HEAD];
    int kind;
    crc = 0;
    if (read(h, sizeof h) != sizeof h || memcmp(h, magic, sizeof magic)) return fail(E_DATA, "not an ac3pack file");
    if (h[AC3P_AT_VERSION] != AC3P_VERSION)
      return fail(E_DATA, "format version %d, which this ac3pack (version %d) doesn't read", h[AC3P_AT_VERSION],
                  AC3P_VERSION);
    if (h[AC3P_AT_LEVEL] != LV::id)
      return fail(E_INTERNAL, "internal error: a stream of level -%d run at level -%d", h[AC3P_AT_LEVEL], LV::id);
    coder_start();
    k.start(this);
    while ((kind = k.code_kind(CK_END)) != CK_END) {
      if (damaged()) return damaged_data();
      if (kind == CK_FRAME) {
        NoBlockCb nb;
        BitOut bo;
        memset((void*)&f, 0, sizeof f);
        if (!k.frame(f) || f.fbytes < 1 || f.fbytes > MAX_FRAME) return damaged_data();
        const int n = f.fbytes, tailbits = f.tailbits;
        bo.init(back, n * BYTE_BITS);
        Walk<BitOut, NoBlockCb> wb(bo, f, nb);
        if (!wb.frame() || bo.pos != n * BYTE_BITS || f.tailbits != tailbits) return damaged_data();
        put_crcs(back, n, f);
        nframe++;
        frame_comes(f);
        write(back, (size_t)n);
        crc = crc32_upd(crc, back, (size_t)n);
      } else {
        size_t n = k.code_raw_len(0), done = 0;
        if (!n || n > IBUF_SIZE || (kind == CK_BAD && n > MAX_FRAME) || damaged()) return damaged_data();
        if (kind == CK_BAD) nframe++;
        while (done < n) {  // a buffer at a time
          const size_t l = n - done < sizeof pbuf ? n - done : sizeof pbuf;
          k.code_raw_bytes(pbuf, l, kind);
          write(pbuf, l);
          crc = crc32_upd(crc, pbuf, l);
          done += l;
          if (damaged()) return damaged_data();
        }
      }
      if (damaged()) return damaged_data();
    }
    // the CRC, the stream's last bytes: the coder has then read all it wrote, and nothing more is
    // there (bytes after it: damaged data too), but in a part's stream, the worker's next record
    if (code_u32(0) != crc || damaged()) return damaged_data();
    if (!is_part && (inpptr < inpend || next_input()))
      return fail(E_DATA, "damaged data: bytes after the end of the stream");
    return 0;
  }
};
