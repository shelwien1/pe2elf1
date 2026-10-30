// m4vpart.inc -- mt.inc's M4vPart: a worker of c -t and of d.

// MTwrap's Enc_class: a worker, coding the parts it is dealt one after the other, each by a model
// of its own (M4vCoro, a coroutine of the worker's, on the part in memory). Plain data, never
// constructed (MTwrap maps its workers): Init() sets it up, do_process() frees what it took as it
// ends. What a part's run fails with is the worker's error (its note).
template <int f_DEC>
struct M4vPart : CoIo {
  MtArgs args;
  U32 worker;      // its place among the workers, dealt the parts in turn
  U64 parts;       // the parts done
  Stats st;        // c: its parts', added up (-s: and the bits by kind of syntax element)
  double cost[NCAT], orig[NCAT];
  U8 *in_, *out_;  // c: a part, all of it; d: IO_BUF of its record at a time. The part's run's output
  size_t in_room, out_room, out_len;
  // its counts on the progress line: the stream's bytes, the compressed stream's and the VOPs of its
  // parts done (done_*, the worker's own), and with the part under way (prog_*: read from any
  // thread)
  U64 done_raw, done_packed, done_vops;
  U64 prog_raw, prog_packed, prog_vops;

  uint Init(const MtArgs* a, uint w) {
    coro_init();
    args = *a;
    worker = w;
    parts = 0;
    memset(&st, 0, sizeof st);
    memset(cost, 0, sizeof cost);
    memset(orig, 0, sizeof orig);
    in_ = out_ = 0;
    in_room = out_room = out_len = 0;
    done_raw = done_packed = done_vops = 0;
    prog_set(0, 0, 0);
    return 0;
  }
  void prog_set(U64 raw, U64 packed, U64 vops) {
    __atomic_store_n(&prog_raw, raw, __ATOMIC_RELAXED);
    __atomic_store_n(&prog_packed, packed, __ATOMIC_RELAXED);
    __atomic_store_n(&prog_vops, vops, __ATOMIC_RELAXED);
  }
  void prog_get(U64& raw, U64& packed, U64& vops) const {
    raw += __atomic_load_n(&prog_raw, __ATOMIC_RELAXED);
    packed += __atomic_load_n(&prog_packed, __ATOMIC_RELAXED);
    vops += __atomic_load_n(&prog_vops, __ATOMIC_RELAXED);
  }
  void Quit(void) {
    free(in_);
    free(out_);
    in_ = out_ = 0;
    in_room = out_room = 0;
  }

  void do_process(void) {
    int r;
    io_init();
    r = f_DEC ? run_d() : run_c();
    Quit();
    for (;;) yield(static_cast<Coroutine*>(this), r ? Y_FAIL : Y_END);
  }

  // the place of the next part it has (worker w's k-th: w + kN)
  U64 next_part(void) const { return worker + parts * (U64)args.threads; }

  // c: a part's statistics, added to the worker's
  void add_stats(const Stats& s, const double* c, const double* o) {
    int i;
    st.vops += s.vops;
    st.packed += s.packed;
    for (i = 0; i < PT_N; i++) st.by_type[i] += s.by_type[i];
    st.raw_unsup += s.raw_unsup;
    st.raw_long += s.raw_long;
    st.raw_parse += s.raw_parse;
    st.raw_verify += s.raw_verify;
    st.raw_bytes += s.raw_bytes;
    st.vop_bytes += s.vop_bytes;
    for (i = 0; i < NCAT; i++) cost[i] += c[i], orig[i] += o[i];
  }

  // A part's run: the coroutine c on the input feed() gives (feed(&p): the next piece at p, its size;
  // 0 at the end), its output into out_ (out_len bytes; most at most, one more fails the part); its
  // status (0, or the exit status of its error, its note copied to the worker's). As it goes, its
  // counts are the worker's (from done_* on), what it tells is shown, and a line of progress may be
  // due.
  template <class C, class Feed>
  int run_part(C* c, Feed feed, U64 most) {
    int r;
    uint y;
    out_len = 0;
    if (!grow(&out_, &out_room, IO_BUF, IO_BUF)) return fail(E_INTERNAL, "out of memory");
    c->addout(out_, IO_BUF);
    for (;;) {
      y = c->coro_call(c);
      if (args.meter) {
        if (y == Y_VIDEO) args.meter->video(c->video);
        prog_set(done_raw + c->raw_done(), done_packed + c->packed_done(), done_vops + c->nvop);
        args.meter->poll();
      }
      if (y == Y_VIDEO) continue;
      if (y == Y_IN) {
        const U8* p = 0;
        const size_t l = feed(&p);
        if (!l) c->f_quit = 1;
        c->addinp((byte*)(l ? p : out_), (uint)l);
        continue;
      }
      out_len += c->getoutsize();
      if (y != Y_OUT) break;
      if (out_len > most) {  // (a part that comes out longer than its record says: d only)
        c->Quit();
        return fail(E_DATA, "damaged data: a part comes out longer than its record says");
      }
      if (!grow(&out_, &out_room, out_len + IO_BUF, IO_BUF)) {
        c->Quit();
        return fail(E_INTERNAL, "out of memory");
      }
      c->addout(out_ + out_len, IO_BUF);
    }
    r = c->status;
    if (r) fail(r, "%s", c->note);
    done_raw += c->raw_done();
    done_packed += c->packed_done();
    done_vops += c->nvop;
    return r;
  }

  // c: each part as the splitter frames it (u64 L, u64 ins, u64 xlen, then the part, L - MT_F_PAST
  // bytes) into its record, and as the parts end, the record of the worker's end
  int run_c(void) {
    for (;;) {
      U8 h[MT_FRAME], rec[MT_REC];
      const size_t n = read(h, MT_FRAME);
      U64 L, ins, xlen, plen;
      int r;
      if (!n) {
        mt_rec(rec, MT_REC, 0, 0, 0, next_part());
        write(rec, MT_REC);
        return 0;
      }
      L = get_le(h + MT_F_LEN, U64_BYTES);
      ins = get_le(h + MT_F_INS, U64_BYTES);
      xlen = get_le(h + MT_F_XLEN, U64_BYTES);
      plen = L - MT_F_PAST;
      if (n < MT_FRAME || L < MT_F_PAST || ins > plen || xlen > plen - ins || plen != (size_t)plen)
        return fail(E_INTERNAL, "internal error: a part framed wrong");
      if (!grow(&in_, &in_room, (size_t)plen + 1, IO_BUF)) return fail(E_INTERNAL, "out of memory");
      if (read(in_, (size_t)plen) != plen) return fail(E_INTERNAL, "internal error: a part cut short");
      r = with_level<PartC>(args.level, *this, plen);
      if (r == kNoLevel) return fail(E_INTERNAL, "no level -%d in this build", args.level);
      if (r) return r;
      mt_rec(rec, MT_REC + out_len, plen, ins, xlen, next_part());
      write(rec, MT_REC);
      write(out_, out_len);
      parts++;
      done_packed += MT_REC;
      done_raw -= xlen;  // (the headers copied in are not the stream's)
      // the record handed over now, its last buffer as far as it is filled (a round's end): so
      // MTwrap puts each record in the container as its part is done, and d, which takes the
      // parts in turn, finds the next within about a round of the others
      yield(static_cast<Coroutine*>(this), Y_PART);
    }
  }

  // d: each record into its part, the headers copied in removed, after which MTwrap takes the
  // next part from the next worker; to the record of the worker's end, after which nothing
  int run_d(void) {
    for (;;) {
      U8 h[MT_REC];
      U8 head[M4V_HEAD];
      U64 g_size, plen, ins, xlen, left;
      size_t nh;
      int r;
      if (read(h, MT_REC) < MT_REC) return fail(E_DATA, "damaged data: a worker's records cut short");
      g_size = get_le(h + MT_R_SIZE, U64_BYTES);
      plen = get_le(h + MT_R_PLEN, U64_BYTES);
      ins = get_le(h + MT_R_INS, U64_BYTES);
      xlen = get_le(h + MT_R_XLEN, U64_BYTES);
      if (get_le(h + MT_R_CRC, U32_BYTES) != mt_rec_crc(h) || get_le(h + MT_R_ZERO, U32_BYTES) ||
          get_le(h + MT_R_PART, U64_BYTES) != next_part())
        return fail(E_DATA, "damaged data: a part's record broken, or a part lost, added or moved");
      if (g_size == MT_REC) {  // the worker's end
        U8 x;
        if (plen || ins || xlen) return fail(E_DATA, "damaged data: a part's record broken");
        if (read(&x, 1)) return fail(E_DATA, "damaged data after a worker's end");
        return 0;
      }
      if (plen > args.part_max || ins > plen || xlen > plen - ins || g_size < MT_REC)
        return fail(E_DATA, "damaged data: a part's record broken");
      // the part compressed, read as its run asks for it, IO_BUF at a time: its header here (its
      // level), then the rest of g_size from the input
      left = g_size - MT_REC;
      nh = (size_t)(left < (U64)M4V_HEAD ? left : (U64)M4V_HEAD);
      if (read(head, nh) < nh || nh < M4V_HEAD) return fail(E_DATA, "damaged data: a part's record cut short");
      left -= nh;
      if (!grow(&in_, &in_room, IO_BUF, IO_BUF)) return fail(E_INTERNAL, "out of memory");
      r = with_level<PartD>(head[M4V_AT_LEVEL], *this, (const U8*)head, nh, plen, left);
      if (r == kNoLevel)
        return fail(E_DATA, "a part compressed at level -%d, which this build has not", head[M4V_AT_LEVEL]);
      if (r) return r;
      if (left) return fail(E_DATA, "damaged data: a part's record broken");  // (its stream ended before it)
      if (out_len != plen) return fail(E_DATA, "damaged data: a part comes out shorter than its record says");
      write(out_, (size_t)ins);
      write(out_ + ins + xlen, (size_t)(plen - ins - xlen));
      parts++;
      done_packed += MT_REC;
      done_raw -= xlen;  // (the headers copied in are not the stream's)

      yield(static_cast<Coroutine*>(this), Y_PART);
    }
  }
};
