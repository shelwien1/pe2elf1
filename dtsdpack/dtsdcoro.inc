// dtsdcoro.inc -- coro.inc's DtsCoro: a run of c or d, at a level.

// A run of dtsdpack: compressing (f_DEC 0) or decompressing (1) a core stream against a WAV, as a
// coroutine, by the model of level LV; the coroutine is the range coder too (coder.inc), whose
// bytes go through its output or come from its input. Plain data, mapped (coro_new(), zeroed) and
// never constructed: Init() sets it up, Quit() frees what it holds. (ac3pack's Ac3Coro.)
template <int f_DEC, class LV>
struct DtsCoro : CoIo<Coder<f_DEC> > {
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
  WavIn* wav;  // the WAV
  Bind bind;   // c: how the core lines up with it (d: from the stream)
  CoreRec f, scratch;                             // the frame being coded; the reader's
  U8 fb[MAX_FRAME + PAD], back[MAX_FRAME + PAD];  // c: the frame (and PAD zero bytes); it written back
  U8 pbuf[IO_BUF];                                // d: bytes kept as bytes, a buffer at a time
  // what the progress line shows: the frames so far, and the audio as told (Y_AUDIO)
  U64 nframe;
  AudioInfo audio;

  void Init(int s, WavIn* w, const Bind& b) {  // (s: c -s, the bits by kind of syntax element)
    coro_init();
    io_init();
    stats = s;
    status = 1;
    nframe = 0;
    wav = w;
    bind = b;
    memset((void*)&audio, 0, sizeof audio);
    k.Init();
  }
  // the stream's bytes and the compressed stream's done so far
  U64 raw_done(void) const { return f_DEC ? out_put() : (U64)st.in_bytes; }
  U64 packed_done(void) const { return f_DEC ? in_taken() : out_put(); }
  // a frame comes as records: where the audio it has is another, the driver is told
  void frame_comes(const CoreRec& r) {
    AudioInfo a;
    a.set(r);
    if (!a.same(audio)) {
      audio = a;
      yield(static_cast<Coroutine*>(this), Y_AUDIO);
    }
  }
  void Quit(void) {
    k.Quit();
    rd.quit();
  }

  void do_process(void) {
    const int r = f_DEC ? run_d() : run_c();
    Quit();
    status = r;
    for (;;) yield(static_cast<Coroutine*>(this), r ? Y_FAIL : Y_END);
  }

  // c: the frame (n bytes at fb, PAD zero bytes after them) parsed into f and written back: 0, or
  // why it can't be coded as records (1: it doesn't parse, 2: it doesn't come back the same)
  int parse(int n) {
    if (core_parse(fb, n, f)) return 1;
    const int m = core_write(f, back, MAX_FRAME);
    return m == n && !memcmp(back, fb, (size_t)n) ? 0 : 2;
  }

  int run_c(void) {
    size_t pos, len;
    int t;
    U8 head[DP_HEAD];
    memset(&st, 0, sizeof st);
    crc = 0;
    if (!rd.init(&scratch)) return fail(E_INTERNAL, "out of memory");
    memcpy(head, magic, sizeof magic);
    head[DP_AT_VERSION] = DP_VERSION;
    head[DP_AT_LEVEL] = (U8)LV::id;
    bind_put(head + DP_AT_BIND, bind, *wav);
    write(head, sizeof head);
    coder_start();
    k.start(this, wav, bind);
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
        if (!why) {
          frame_comes(f);
          k.code_kind(CK_FRAME);
          if (!k.frame(f, (int)len)) return fail(E_INTERNAL, "internal error: a frame parsed couldn't be coded");
          k.frame_bytes(fb, (int)len);
          st.packed++;
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
      if (kind == CK_BAD) k.skip_blocks(core_blocks(p, len));
    }
    k.code_kind(CK_END);
    code_u32(crc);
    coder_end();
#ifdef DTSD_FSTAT
    if (stats)
      for (int i = 0; i <= F_N; i++)
        if (B::fcost[i] > 0) fprintf(stderr, "  %-20s %10.0f\n", i < F_N ? field_name[i] : "(container)", B::fcost[i] / 8);
#endif
    st.hits = k.n_hit, st.smp = k.n_smp, st.sf_hits = k.n_sfhit, st.sf = k.n_sf;
    st.hits9 = k.n_hit9, st.smp9 = k.n_smp9, st.sf_hits9 = k.n_sfhit9, st.sf9 = k.n_sf9;
    st.ab = k.n_ab, st.ab_srch = k.n_abs, st.ab_hits = k.n_abhit;
    st.ab_st = k.n_st, st.ab_sthits = k.n_sthit;
    for (int i = 0; i < VK_N; i++) st.vq[i] = k.n_vq[i], st.vq_hits[i] = k.n_vqhit[i];
    st.pv = k.n_pv, st.pv_hits = k.n_pvhit;
    return 0;
  }

  // d: the data is damaged (said: the stream cut short, where the coder had to read past its end)
  int damaged_data(void) { return fail(E_DATA, f_quit ? "damaged data: the stream is cut short" : "damaged data"); }

  int run_d(void) {
    U8 h[DP_HEAD];
    int kind;
    crc = 0;
    if (read(h, sizeof h) != sizeof h || memcmp(h, magic, sizeof magic)) return fail(E_DATA, "not a dtsdpack file");
    if (h[DP_AT_VERSION] != DP_VERSION)
      return fail(E_DATA, "format version %d, which this dtsdpack (version %d) doesn't read", h[DP_AT_VERSION], DP_VERSION);
    if (h[DP_AT_LEVEL] != LV::id)
      return fail(E_INTERNAL, "internal error: a stream of level -%d run at level -%d", h[DP_AT_LEVEL], LV::id);
    if (const char* why = bind_differs(h + DP_AT_BIND, *wav))
      return fail(E_DATA, "the WAV isn't the one it was compressed with (%s differs)", why);
    bind = bind_get(h + DP_AT_BIND);
    coder_start();
    k.start(this, wav, bind);
    while ((kind = k.code_kind(CK_END)) != CK_END) {
      if (damaged()) return damaged_data();
      if (kind == CK_FRAME) {
        if (!k.frame(f, 0)) return damaged_data();
        const int n = core_write(f, back, MAX_FRAME);
        if (!n) return damaged_data();
        k.frame_bytes(back, n);
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
          if (kind == CK_BAD) k.skip_blocks(core_blocks(pbuf, l));  // (all of it: a frame fits the buffer)
          write(pbuf, l);
          crc = crc32_upd(crc, pbuf, l);
          done += l;
          if (damaged()) return damaged_data();
        }
      }
      if (damaged()) return damaged_data();
    }
    // the CRC, the stream's last bytes: the coder has then read all it wrote, and nothing more is
    // there (bytes after it: damaged data too)
    if (code_u32(0) != crc || damaged()) return damaged_data();
    if (inpptr < inpend || next_input()) return fail(E_DATA, "damaged data: bytes after the end of the stream");
    return 0;
  }
};
