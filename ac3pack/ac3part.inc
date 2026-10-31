// ac3part.inc -- mt.inc's Ac3Part: a worker of c -t and of d.

// MTwrap's Enc_class: a worker, coding the parts it is dealt one after the other, each by a model
// of its own (Ac3Coro, a coroutine of the worker's): c on the part in memory, its output written as
// it comes; d on the worker's input as it comes, its output in memory until the part is done (the
// parts go out in turn). Plain data, never constructed (MTwrap maps its workers): Init() sets it
// up, do_process() frees what it took as it ends. What a part's run fails with is the worker's
// error (its note).
template <int f_DEC>
struct Ac3Part : CoIo<Coroutine> {
  MtArgs args;
  U32 worker;      // its place among the workers, dealt the parts in turn
  U64 parts;       // the parts done
  Stats st;        // c: its parts', added up (-s: and the bits by kind of syntax element)
  double cost[NCAT], orig[NCAT];
  U8 *in_, *out_;  // c: a part, all of it. The part's run's output: c, IO_BUF at a time; d, all of it
  size_t in_room, out_room, out_len;
  // its counts on the progress line: the stream's bytes, the compressed stream's and the frames of
  // its parts done (done_*, the worker's own), and with the part under way (prog_*: read from any
  // thread)
  U64 done_raw, done_packed, done_frames;
  U64 prog_raw, prog_packed, prog_frames;

  uint Init(rp<const MtArgs> a, uint w) {
    coro_init();
    io_init();
    args = *a;
    worker = w;
    parts = 0;
    memset(&st, 0, sizeof st);
    memset(cost, 0, sizeof cost);
    memset(orig, 0, sizeof orig);
    in_ = out_ = 0;
    in_room = out_room = out_len = 0;
    done_raw = done_packed = done_frames = 0;
    prog_set(0, 0, 0);
    return 0;
  }
  void prog_set(U64 raw, U64 packed, U64 frames) {
    __atomic_store_n(&prog_raw, raw, __ATOMIC_RELAXED);
    __atomic_store_n(&prog_packed, packed, __ATOMIC_RELAXED);
    __atomic_store_n(&prog_frames, frames, __ATOMIC_RELAXED);
  }
  void prog_get(U64& raw, U64& packed, U64& frames) const {
    raw += __atomic_load_n(&prog_raw, __ATOMIC_RELAXED);
    packed += __atomic_load_n(&prog_packed, __ATOMIC_RELAXED);
    frames += __atomic_load_n(&prog_frames, __ATOMIC_RELAXED);
  }
  void Quit(void) {
    free(in_);
    free(out_);
    in_ = out_ = 0;
    in_room = out_room = 0;
  }

  void do_process(void) {
    int r;
    r = f_DEC ? run_d() : run_c();
    Quit();
    for (;;) yield(static_cast<Coroutine*>(this), r ? Y_FAIL : Y_END);
  }

  // the place of the next part it has (worker w's k-th: w + kN)
  U64 next_part(void) const { return worker + parts * (U64)args.threads; }

  // c: a part's statistics, added to the worker's
  void add_stats(const Stats& s, rp<const double> c, rp<const double> o) {
    int i;
    st.add(s);
    for (i = 0; i < NCAT; i++) cost[i] += c[i], orig[i] += o[i];
  }

  // A part's run: the coroutine c on the input feed() gives (feed(&p): the next piece at p, its size;
  // 0 at the end), its output (c) into the worker's output a buffer at a time, as it comes, or (d)
  // into out_ (out_len bytes; most at most, one more fails the part); its status (0, or the exit
  // status of its error, its note copied to the worker's). As it goes, its counts are the worker's
  // (from done_* on), what it tells is shown, and a line of progress may be due.
  template <class C, class Feed>
  int run_part(C& c, Feed feed, U64 most) {
    int r;
    uint y;
    out_len = 0;
    if (!grow(&out_, &out_room, IO_BUF, IO_BUF)) return fail(E_INTERNAL, "out of memory");
    c.addout(out_, IO_BUF);
    for (;;) {
      y = c.coro_call(&c);
      if (args.meter) {
        Progress& m = *args.meter;
        if (y == Y_AUDIO) m.audio(c.audio);
        prog_set(done_raw + c.raw_done(), done_packed + c.packed_done(), done_frames + c.nframe);
        m.poll();
      }
      if (y == Y_AUDIO) continue;
      if (y == Y_IN) {
        const U8* p = 0;
        const size_t l = feed(&p);
        if (!l) c.f_quit = 1;
        c.addinp((byte*)(l ? p : out_), (uint)l);
        continue;
      }
      if (!f_DEC) {  // c: on to the container as it comes (MTwrap puts each block of the worker's in it)
        write(out_, c.getoutsize());
        if (y != Y_OUT) break;
        c.addout(out_, IO_BUF);
        continue;
      }
      out_len += c.getoutsize();
      if (y != Y_OUT) break;
      if (out_len > most) {  // (a part that comes out longer than its record says)
        c.Quit();
        return fail(E_DATA, "damaged data: a part comes out longer than its record says");
      }
      if (!grow(&out_, &out_room, out_len + IO_BUF, IO_BUF)) {
        c.Quit();
        return fail(E_INTERNAL, "out of memory");
      }
      c.addout(out_ + out_len, IO_BUF);
    }
    r = c.status;
    if (r) fail(r, "%s", c.note);
    done_raw += c.raw_done();
    done_packed += c.packed_done();
    done_frames += c.nframe;
    return r;
  }

  // c: each part as the splitter frames it (u64 L, then the part, L bytes) into its record, the
  // part compressed as it comes; and as the parts end, the record of the worker's end
  int run_c(void) {
    for (;;) {
      U8 h[MT_FRAME], rec[MT_REC];
      const size_t n = read(h, MT_FRAME);
      U64 plen;
      int r;
      if (!n) {
        mt_rec(rec, 0, next_part());
        write(rec, MT_REC);
        return 0;
      }
      plen = get_le(h + MT_F_LEN, U64_BYTES);
      if (n < MT_FRAME || !plen || plen != (size_t)plen)
        return fail(E_INTERNAL, "internal error: a part framed wrong");  // (plen 0 is the worker's end)
      if (!grow(&in_, &in_room, (size_t)plen + 1, IO_BUF)) return fail(E_INTERNAL, "out of memory");
      if (read(in_, (size_t)plen) != plen) return fail(E_INTERNAL, "internal error: a part cut short");
      mt_rec(rec, plen, next_part());
      write(rec, MT_REC);
      r = with_level<PartC>(args.level, *this, plen);
      if (r == kNoLevel) return fail(E_INTERNAL, "no level -%d in this build", args.level);
      if (r) return r;
      parts++;
      done_packed += MT_REC;
      // the part's last buffer handed over now, as far as it is filled (a round's end): so the
      // part's end is in the container as it is done, not after some of the next part
      yield(static_cast<Coroutine*>(this), Y_PART);
    }
  }

  // d: a part's run reads the worker's input where it is: lend() hands it the rest of the input
  // buffer (*p, its size; 0: the input has ended) as if it were read, and take_back() takes back
  // what the run left of it (past the part's stream: the next record)
  size_t lend(rp<const U8*> p) {
    size_t l;
    if (inpptr >= inpend && !next_input()) return 0;
    *p = inpptr;
    l = (size_t)(inpend - inpptr);
    inpptr = inpend;
    return l;
  }
  template <class C>
  void take_back(const C& c) {
    if (c.inpend == inpend && c.inpptr <= inpend) inpptr = c.inpptr;
  }

  // d: each record into its part, after which MTwrap takes the next part from the next worker; to
  // the record of the worker's end, after which nothing
  int run_d(void) {
    for (;;) {
      U8 h[MT_REC];
      U8 head[AC3P_HEAD];
      U64 plen;
      int r;
      if (read(h, MT_REC) < MT_REC) return fail(E_DATA, "damaged data: a worker's records cut short");
      plen = get_le(h + MT_R_PLEN, U64_BYTES);
      if (get_le(h + MT_R_CRC, U32_BYTES) != mt_rec_crc(h) || get_le(h + MT_R_ZERO, U32_BYTES) ||
          get_le(h + MT_R_PART, U64_BYTES) != next_part())
        return fail(E_DATA, "damaged data: a part's record broken, or a part lost, added or moved");
      if (!plen) {  // the worker's end
        U8 x;
        if (read(&x, 1)) return fail(E_DATA, "damaged data after a worker's end");
        return 0;
      }
      if (plen > args.part_max) return fail(E_DATA, "damaged data: a part's record broken");
      // the part compressed: its header here (its level), the rest read by its run where it is
      if (read(head, AC3P_HEAD) < AC3P_HEAD) return fail(E_DATA, "damaged data: a part's record cut short");
      r = with_level<PartD>(head[AC3P_AT_LEVEL], *this, (const U8*)head, plen);
      if (r == kNoLevel)
        return fail(E_DATA, "a part compressed at level -%d, which this build has not", head[AC3P_AT_LEVEL]);
      if (r) return r;
      if (out_len != plen) return fail(E_DATA, "damaged data: a part comes out shorter than its record says");
      write(out_, (size_t)plen);
      parts++;
      done_packed += MT_REC;

      yield(static_cast<Coroutine*>(this), Y_PART);
    }
  }
};
