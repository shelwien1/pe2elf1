// m4vcoro.inc -- coro.inc's M4vCoro: a run of c or d, at a level.

// A run of m4vpack: compressing (f_DEC 0) or decompressing (1) a stream, as a coroutine, by the
// model of level LV. Plain data, mapped (coro_new(), zeroed) and never constructed: Init() sets it
// up, Quit() frees what it holds. A run shares nothing with another but the constant tables, so
// any number of them can go at once, on as many threads.
template <int f_DEC, class LV>
struct M4vCoro : CoIo {
  static constexpr int f_dec = f_DEC;
  int stats;   // c -s: the bits by kind of syntax element (co.cost, co.orig)
  int status;  // the run's: 0, or the exit status of its error (note); 1 until it has ended
  Stats st;    // c's
  U32 crc;     // of the stream: c's input, d's output
  Coder co;
  Reader rd;   // c: the input
  Ctx cx;
  Model<f_DEC, LV> k;
  U8 pbuf[IO_BUF];  // a VOP written back (W_VERIFY, W_DECODE), a buffer at a time
  // what the progress line shows: the VOPs so far, and the video as told (Y_VIDEO) with the VOL it
  // was told for (Ctx::vol_count)
  U64 nvop;
  VideoInfo video;
  int told_vol;

  void Init(int s) {
    coro_init();
    stats = s;
    status = 1;
    nvop = 0;
    told_vol = 0;
    memset((void*)&video, 0, sizeof video);
    k.Init();
  }
  // the stream's bytes and the compressed stream's done so far (c: the chunks taken, as they start)
  U64 raw_done(void) const { return f_DEC ? n_out : (U64)st.in_bytes; }
  U64 packed_done(void) const { return f_DEC ? n_in : n_out; }
  // a VOP comes: counted, and where it is the first of a VOL (by then the encoder's user data has
  // come too), what the video is told
  void vop_comes(void) {
    nvop++;
    if (cx.have_vol && cx.vol_count != told_vol) {
      told_vol = cx.vol_count;
      video.set(cx);
      yield(static_cast<Coroutine*>(this), Y_VIDEO);
    }
  }
  void Quit(void) {
    cx.quit();
    k.Quit();
    rd.quit();
  }

  void do_process(void) {
    int r;
    io_init();
    r = f_DEC ? run_d() : run_c();
    Quit();
    status = r;
    for (;;) yield(static_cast<Coroutine*>(this), r ? Y_FAIL : Y_END);
  }

  int run_c(void) {
    size_t pos, len;
    int part, r;
    memset(&st, 0, sizeof st);
    crc = 0;
    U8 head[M4V_HEAD];
    cx.init();
    if (!rd.init(this)) return fail(E_INTERNAL, "out of memory");
    memcpy(head, magic, sizeof magic);
    head[M4V_AT_VERSION] = M4V_VERSION;
    head[M4V_AT_LEVEL] = (U8)LV::id;
    write(head, sizeof head);
    co.init(this, 1);
    co.stats = stats;
    k.start(&co);
    while (rd.chunk(&pos, &len, &part)) {
      U8* p = rd.buf + pos;
      const int is_vop = len >= SC_LEN && sc_code(p) == SC_VOP;
      crc = crc32_upd(crc, p, len);
      st.in_bytes += (double)len;
      if (is_vop && part != PC_MORE) {
        vop_comes();
        st.vops++;
        if (len > SC_LEN) st.by_type[vop_type(p)]++;
        if (part) st.raw_long++;
        else if (!cx.have_vol || cx.unsupported || (cx.dp && (len <= SC_LEN || vop_type(p) != PT_B))) st.raw_unsup++;
        else {
          const int tib = cx.time_inc_bits;
          GB gb;
          PB pb;
          Cmp cm;
          gb.init(p + SC_LEN, len - SC_LEN);
          pb.init(pbuf, sizeof pbuf);
          cm.p = p;
          cm.n = len;
          cm.at = 0;
          cm.differs = 0;
          pb.flush = Cmp::flush;
          pb.ctx = &cm;
          r = k.vop_walk(cx, W_VERIFY, gb, pb);
          if (r == W_NOMEM) return fail(E_INTERNAL, "out of memory");
          if (r < 0) st.raw_parse++;
          else {
            pb.finish();
            if (pb.err || pb.n || cm.differs || cm.at != len) st.raw_verify++;
            else {
              cx.time_inc_bits = tib;  // the parse again sees the same state
              gb.pos = 0;
              k.code_kind(CK_VOP);
              r = k.vop_walk(cx, W_ENCODE, gb, pb);
              if (r == W_NOMEM) return fail(E_INTERNAL, "out of memory");
              if (r < 0) return fail(E_INTERNAL, "internal error: a verified VOP couldn't be coded");
              st.packed++;
              st.vop_bytes += (double)len;
              continue;
            }
          }
        }
      }
      if (is_vop) st.raw_bytes += (double)len;
      co.orig[CAT_RAW] += (double)BYTE_BITS * len;
      k.code_kind(CK_RAW);
      k.code_raw_len(len);
      k.code_raw_bytes(p, len);
      cx.parse_header_chunk(p, len);
    }
    k.code_kind(CK_END);
    co.code_u32(crc);
    co.flush();
    return 0;
  }

  static void out_flush(void* ctx, const U8* p, size_t n) {  // d: a VOP's bytes, out
    M4vCoro& c = *(M4vCoro*)ctx;
    c.write(p, n);
    c.crc = crc32_upd(c.crc, p, n);
  }

  int run_d(void) {
    U8 h[M4V_HEAD];
    int kind, r;
    crc = 0;
    if (read(h, sizeof h) != sizeof h || memcmp(h, magic, sizeof magic)) return fail(E_DATA, "not an m4vpack file");
    if (h[M4V_AT_VERSION] != M4V_VERSION)
      return fail(E_DATA, "format version %d, which this m4vpack (version %d) doesn't read", h[M4V_AT_VERSION],
                  M4V_VERSION);
    if (h[M4V_AT_LEVEL] != LV::id)
      return fail(E_INTERNAL, "internal error: a stream of level -%d run at level -%d", h[M4V_AT_LEVEL], LV::id);
    cx.init();
    co.init(this, 0);
    k.start(&co);
    while ((kind = k.code_kind(CK_END)) != CK_END) {
      if (co.err) return fail(E_DATA, "damaged data");
      if (kind == CK_VOP) {
        GB gb = GB();  // (decoding reads no bits: the VOP comes out of the model)
        PB pb;
        vop_comes();
        pb.init(pbuf, sizeof pbuf);
        pb.flush = out_flush;
        pb.ctx = this;
        r = k.vop_walk(cx, W_DECODE, gb, pb);
        if (r == W_NOMEM) return fail(E_INTERNAL, "out of memory");
        if (r < 0 || pb.err || pb.n) return fail(E_DATA, "damaged data");
        pb.finish();
      } else {
        size_t n = k.code_raw_len(0), done = 0;
        if (!n || n > IBUF_SIZE || co.err) return fail(E_DATA, "damaged data");
        while (done < n) {  // a buffer at a time; the first has what parse_header_chunk() reads
          const size_t l = n - done < sizeof pbuf ? n - done : sizeof pbuf;
          k.code_raw_bytes(pbuf, l);
          if (!done) {
            cx.parse_header_chunk(pbuf, n);
            if (l >= SC_LEN && sc_code(pbuf) == SC_VOP) vop_comes();
          }
          write(pbuf, l);
          crc = crc32_upd(crc, pbuf, l);
          done += l;
          if (over > OVER_MAX) return fail(E_DATA, "damaged data");
        }
      }
      if (over > OVER_MAX) return fail(E_DATA, "damaged data");
    }
    if (co.err || co.code_u32(0) != crc || over > OVER_MAX) return fail(E_DATA, "damaged data");
    return 0;
  }
};
