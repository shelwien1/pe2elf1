// coro.inc -- the compressor and the decompressor, as coroutines (Lib3's, as h264p's): a run,
// on the buffers of its input and output, at a level.

// The stream: "m4vpack", the format's version, the model's level; then the arithmetic coded data:
// the chunks (kind, then the records of a VOP or a raw chunk's length and bytes), the end, and
// the CRC-32 of the stream (32 bits at 1/2 each).
enum { M4V_MAGIC = 7, M4V_AT_VERSION = M4V_MAGIC, M4V_AT_LEVEL, M4V_HEAD };  // the header's bytes
static const U8 magic[M4V_MAGIC] = { 'm', '4', 'v', 'p', 'a', 'c', 'k' };
constexpr int M4V_VERSION = 4;  // the format's

// a compression's statistics (its summary line)
typedef struct Stats {
  U32 vops, packed, by_type[PT_N], raw_unsup, raw_long, raw_parse, raw_verify;
  double in_bytes, raw_bytes, vop_bytes;
  U64 parts;    // -t: the parts
  int threads;  // and the threads
} Stats;

// what W_VERIFY's VOP written back is compared with, as its writer flushes it
typedef struct Cmp {
  const U8* p;
  size_t n, at;
  int differs;
} Cmp;
static void cmp_flush(void* ctx, const U8* p, size_t n) {
  Cmp* c = (Cmp*)ctx;
  if (c->differs || n > c->n - c->at || memcmp(c->p + c->at, p, n)) c->differs = 1;
  else c->at += n;
}

// A run of m4vpack: compressing (f_DEC 0) or decompressing (1) a stream, as a coroutine, by the
// model of level LV. Plain data, mapped (coro_new(), zeroed) and never constructed: Init() sets it
// up, Quit() frees what it holds. A run shares nothing with another but the constant tables, so
// any number of them can go at once, on as many threads.
template <int f_DEC, class LV>
struct M4vCoro : CoIo {
  int stats;   // c -s: the bits by kind of syntax element (co.cost, co.orig)
  int status;  // the run's: 0, or the exit status of its error (note); 1 until it has ended
  Stats st;    // c's
  U32 crc;     // of the stream: c's input, d's output
  Coder co;
  Reader rd;   // c: the input
  Ctx cx;
  Model<f_DEC, LV> k;
  U8 pbuf[IO_BUF];  // a VOP written back (W_VERIFY, W_DECODE), a buffer at a time

  void Init(int s) {
    coro_init();
    stats = s;
    status = 1;
    k.Init();
  }
  void Quit(void) {
    ctx_quit(&cx);
    k.Quit();
    free(rd.buf);
    rd.buf = 0;
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
    ctx_init(&cx);
    if (!rd_init(&rd, this)) return fail(E_INTERNAL, "out of memory");
    memcpy(head, magic, sizeof magic);
    head[M4V_AT_VERSION] = M4V_VERSION;
    head[M4V_AT_LEVEL] = (U8)LV::id;
    write(head, sizeof head);
    co_init(&co, this, 1);
    co.stats = stats;
    k.start(&co);
    while (rd_chunk(&rd, &pos, &len, &part)) {
      U8* p = rd.buf + pos;
      const int is_vop = len >= SC_LEN && sc_code(p) == SC_VOP;
      crc = crc32_upd(crc, p, len);
      st.in_bytes += (double)len;
      if (is_vop && part != PC_MORE) {
        st.vops++;
        if (len > SC_LEN) st.by_type[vop_type(p)]++;
        if (part) st.raw_long++;
        else if (!cx.have_vol || cx.unsupported || (cx.dp && (len <= SC_LEN || vop_type(p) != PT_B))) st.raw_unsup++;
        else {
          const int tib = cx.time_inc_bits;
          GB gb;
          PB pb;
          Cmp cm;
          gb.buf = p + SC_LEN;
          gb.size = (int)(len - SC_LEN) * BYTE_BITS;
          gb.pos = 0;
          pb_init(&pb, pbuf, sizeof pbuf);
          cm.p = p;
          cm.n = len;
          cm.at = 0;
          cm.differs = 0;
          pb.flush = cmp_flush;
          pb.ctx = &cm;
          r = k.vop_walk(&cx, W_VERIFY, &gb, &pb);
          if (r == W_NOMEM) return fail(E_INTERNAL, "out of memory");
          if (r < 0) st.raw_parse++;
          else {
            pb_finish(&pb);
            if (pb.err || pb.n || cm.differs || cm.at != len) st.raw_verify++;
            else {
              cx.time_inc_bits = tib;  // the parse again sees the same state
              gb.pos = 0;
              k.code_kind(CK_VOP);
              r = k.vop_walk(&cx, W_ENCODE, &gb, 0);
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
      parse_header_chunk(&cx, p, len);
    }
    k.code_kind(CK_END);
    code_u32(&co, crc);
    co_flush(&co);
    return 0;
  }

  static void out_flush(void* ctx, const U8* p, size_t n) {  // d: a VOP's bytes, out
    M4vCoro* c = (M4vCoro*)ctx;
    c->write(p, n);
    c->crc = crc32_upd(c->crc, p, n);
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
    ctx_init(&cx);
    co_init(&co, this, 0);
    k.start(&co);
    while ((kind = k.code_kind(CK_END)) != CK_END) {
      if (co.err) return fail(E_DATA, "damaged data");
      if (kind == CK_VOP) {
        PB pb;
        pb_init(&pb, pbuf, sizeof pbuf);
        pb.flush = out_flush;
        pb.ctx = this;
        r = k.vop_walk(&cx, W_DECODE, 0, &pb);
        if (r == W_NOMEM) return fail(E_INTERNAL, "out of memory");
        if (r < 0 || pb.err || pb.n) return fail(E_DATA, "damaged data");
        pb_finish(&pb);
      } else {
        size_t n = k.code_raw_len(0), done = 0;
        if (!n || n > IBUF_SIZE || co.err) return fail(E_DATA, "damaged data");
        while (done < n) {  // a buffer at a time; the first has what parse_header_chunk() reads
          const size_t l = n - done < sizeof pbuf ? n - done : sizeof pbuf;
          k.code_raw_bytes(pbuf, l);
          if (!done) parse_header_chunk(&cx, pbuf, n);
          write(pbuf, l);
          crc = crc32_upd(crc, pbuf, l);
          done += l;
          if (over > OVER_MAX) return fail(E_DATA, "damaged data");
        }
      }
      if (over > OVER_MAX) return fail(E_DATA, "damaged data");
    }
    if (co.err || code_u32(&co, 0) != crc || over > OVER_MAX) return fail(E_DATA, "damaged data");
    return 0;
  }
};

// A run's coroutine, on pages of its own: zeroed, and taking memory only where the run touches
// them (its tables for the largest pictures). coro_new() makes it (plain data: then Init()),
// coro_free() frees it after its Quit().
template <class T>
T* coro_new(void) {
  return (T*)VirtualAlloc(0, sizeof(T), MEM_COMMIT, PAGE_READWRITE);
}
template <class T>
void coro_free(T* p) {
  if (p) VirtualFree(p, 0, MEM_RELEASE);
}

// The output of a call: a file, or (c -v) the input to compare with
typedef struct Sink {
  FILE* f;
  int compare;
  U64 pos, diff_at;
  int differs, err;
} Sink;
static void sink_write(Sink* s, const U8* p, size_t n) {
  static U8 cmp[IO_BUF];  // (the main thread's)
  if (!s->compare) {
    if (n && fwrite(p, 1, n, s->f) != n) s->err = 1;
    s->pos += n;
    return;
  }
  while (n && !s->differs) {
    const size_t k = n < sizeof(cmp) ? n : sizeof(cmp);
    const size_t got = fread(cmp, 1, k, s->f);
    size_t i;
    for (i = 0; i < got && cmp[i] == p[i]; i++) {}
    if (i < k) { s->differs = 1; s->diff_at = s->pos + i; return; }
    s->pos += k;
    p += k;
    n -= k;
  }
}

// Runs the coroutine c in the buffer protocol to its end: in() gives it its next input when it
// asks (Y_IN), out(more) takes what it put in its output buffer when that is full (Y_OUT, more:
// and gives it the next) or as it ends (Y_END, or Y_FAIL after an error: what drive() returns)
template <class C, class In, class Out>
uint drive(C& c, In in, Out out) {
  for (;;) {
    const uint r = c.coro_call(&c);
    if (r == Y_IN) {
      in();
      continue;
    }
    out(r == Y_OUT);
    if (r != Y_OUT) return r;
  }
}

// c on files: its input from f (the n bytes at pre read already, first), its output to s
static U8 io_bufs[2][IO_BUF];  // (the main thread's: input, output)
template <class C>
uint run_file(C& c, FILE* f, Sink* s, const U8* pre = 0, size_t n = 0) {
  const uint kIo = sizeof io_bufs[0];
  U8 *ib = io_bufs[0], *ob = io_bufs[1];
  c.addout(ob, kIo);
  if (n) c.addinp((byte*)pre, (uint)n);
  return drive(
      c,
      [&]() {
        const uint l = (uint)fread(ib, 1, kIo, f);
        if (!l) c.f_quit = 1;
        c.addinp(ib, l);
      },
      [&](bool more) {
        sink_write(s, c.outbeg, c.getoutsize());
        if (more) c.addout(ob, kIo);
      });
}

