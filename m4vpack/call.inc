// call.inc -- m4vpack.cpp's Call: a call's files, its progress line, its runs; d and c on it.

// A call's input (in, named in_name), its output (the sink s, named out_name: a file, or for -v's
// check the input to compare with), its progress line and what its run did; d and c on it
struct Call {
  FILE* in;
  const char* in_name;
  Sink s;
  const char* out_name;
  Progress pr;
  RunCounts n;

  // from the file from to to (compare: s compares with to instead of writing it), its progress line
  // tagged tag: shown with -p, or where stderr is a terminal, but not with -q
  void init(FILE* from, const char* from_name, FILE* to, const char* to_name, int compare, const Opts& o,
            const char* tag) {
    in = from;
    in_name = from_name;
    memset(&s, 0, sizeof s);
    s.f = to;
    s.compare = compare;
    out_name = to_name;
    memset(&n, 0, sizeof n);
    pr.Init();
    if (o.progress || (!o.quiet && stderr_tty())) pr.start(tag, file_size(in));
  }
  // c's or d's call from the file named from to the one named to: 0, or the exit status of an error
  // (said)
  int open(const char* from, const char* to, const Opts& o, const char* tag) {
    FILE *f, *t;
    if (!(f = open_file(from, 0))) {
      fprintf(stderr, "m4vpack: can't open %s\n", from);
      return E_READ;
    }
    if (!(t = open_file(to, 1))) {
      fprintf(stderr, "m4vpack: can't create %s\n", to);
      if (f != stdin) fclose(f);
      return E_WRITE;
    }
    init(f, from, t, to, 0, o, tag);
    return 0;
  }
  // the end of the call's progress line, its run's status r
  void end(int r) {
    pr.end(r, n.raw, n.packed, n.vops);
    pr.Quit();
  }
  // the end of c's or d's call, its run's status r: the output flushed and closed (the input stays
  // open: -v reads it again), the progress line ended; r, or the exit status of an error writing
  // (said)
  int close(int r) {
    const int flushed = fflush(s.f) == 0, closed = s.f == stdout || fclose(s.f) == 0;
    if (!(flushed && closed) && !r) r = (pr.say("m4vpack: %s: write error\n", out_name), E_WRITE);
    end(r);
    return r;
  }

  // The run c (a coroutine) on the call's files, the bytes at head (got of them) read already first:
  // its progress line's counts f(ctx) as it goes, show(code) after each yield; what run_file() returns
  template <class C, class Show>
  uint run(C& c, ProgressCounts f, void* ctx, Show show, const U8* head = 0, size_t got = 0) {
    uint y;
    pr.counts(f, ctx);
    y = run_file(c, in, s, show, head, got);
    pr.counts(nullptr, nullptr);
    return y;
  }
  // the end of a run on the call's files: its status r (0, or the exit status of an error the run's
  // note says), unless reading or writing failed; said after the progress line
  int run_end(int r, const char* note) {
    if (ferror(in)) {
      pr.say("m4vpack: %s: read error\n", in_name);
      return E_READ;
    }
    if (s.err) {
      pr.say("m4vpack: %s: write error\n", out_name);
      return E_WRITE;
    }
    if (r) pr.say("m4vpack: %s: %s\n", in_name, note);
    return r;
  }
  // a run of one model (C: M4vCoro) on the call's files: its counts into n, its status (said)
  template <class C>
  int run_one(C& c, const U8* head, size_t got) {
    run(c, coro_counts<C>, &c, [&](uint code) { run_show(pr, c, code); }, head, got);
    n.raw = c.raw_done();
    n.packed = c.packed_done();
    n.vops = c.nvop;
    return run_end(c.status, c.note);
  }
  // what MTwrap's Init() returned, said (0: nothing to say)
  int threads_failed(uint r, int threads) {
    if (r == MTW_NO_THREAD) pr.say("m4vpack: can't start %d threads\n", threads);
    else if (r) pr.say("m4vpack: no memory for the buffers of %d threads\n", threads);
    return r != 0;
  }

  // --- d

  // d of a stream of parts (the bytes at head, got of them, read already)
  int decode_mt(const U8* head, size_t got) {
    const int threads = got >= MT_HEAD ? head[MT_AT_THREADS] : 0;
    char note[NOTE_MAX] = "damaged data";
    MtArgs a;
    MtDec* p;
    uint y;
    int r;
    if (got > MT_AT_VERSION && head[MT_AT_VERSION] != M4V_VERSION) {
      pr.say("m4vpack: %s: format version %d, which this m4vpack (version %d) doesn't read\n", in_name,
             head[MT_AT_VERSION], M4V_VERSION);
      return E_DATA;
    }
    if (threads < 1 || threads > MAX_THREADS) return run_end(E_DATA, note);
    a.threads = threads;
    a.stats = 0;
    a.level = 0;
    a.part_max = mt_part_max((get_le(head + MT_AT_PART, MT_PART_BYTES) + 1) * KIB);  // (the part size in KB, rounded down)
    a.meter = &pr;
    if (!(p = coro_new<MtDec>())) no_memory();
    MtDec& d = *p;
    if (threads_failed(d.Init(threads, MT_RING, &a, 1), threads)) return E_INTERNAL;
    y = run(d, mt_counts<MtDec, 1>, &d, [&](uint) { pr.poll(); }, head + MT_HEAD, got - MT_HEAD);
    if (!(r = mt_failed(d, note, sizeof note)) && (y != Y_END || !mt_dealt(d))) r = E_DATA;
    mt_sums(d, n.raw, n.packed, n.vops);
    n.packed = d.c_inp + MT_HEAD;
    d.Quit();  // (the threads joined)
    coro_free(p);
    return run_end(r, note);
  }

#include "decodeone.inc"  // DecodeOne: d of one model's stream

  // d of the call's input into its output: 0, or the exit status of an error (said). Its first bytes
  // say which it is, a stream of one model (and its level) or of parts on threads (mt_magic).
  int decode_to() {
    U8 head[HEAD_MAX];
    const size_t got = fread(head, 1, sizeof head, in);
    int r;
    memset(&n, 0, sizeof n);
    if (got >= sizeof mt_magic && !memcmp(head, mt_magic, sizeof mt_magic)) return decode_mt(head, got);
    if (got < M4V_HEAD || memcmp(head, magic, sizeof magic)) {
      if (ferror(in)) return run_end(0, "");
      pr.say("m4vpack: %s: not an m4vpack file\n", in_name);
      return E_DATA;
    }
    if (head[M4V_AT_VERSION] != M4V_VERSION) {
      pr.say("m4vpack: %s: format version %d, which this m4vpack (version %d) doesn't read\n", in_name,
             head[M4V_AT_VERSION], M4V_VERSION);
      return E_DATA;
    }
    r = with_level<DecodeOne>(head[M4V_AT_LEVEL], *this, (const U8*)head, got);
    if (r != kNoLevel) return r;
    pr.say("m4vpack: %s: compressed at level -%d, which this build has not\n", in_name, head[M4V_AT_LEVEL]);
    return E_DATA;
  }

  // --- c

#include "compressone.inc"  // CompressOne: c on one model
  int compress_one(const Opts& o, Stats& st, double* cost, double* orig) {
    return with_level<CompressOne>(o.level, *this, o, st, cost, orig);
  }

  // c -t N: the stream in parts on N threads, its statistics the parts' added up
  int compress_mt(const Opts& o, Stats& st, double* cost, double* orig) {
    char note[NOTE_MAX] = "internal error: a thread stopped early";
    U8 head[MT_HEAD];
    MtArgs a;
    MtEnc* p;
    uint y, i;
    int r, j;
    a.threads = o.threads;
    a.stats = o.stats;
    a.level = o.level;
    a.part_max = 0;
    a.meter = &pr;
    if (!(p = coro_new<MtEnc>())) no_memory();
    MtEnc& e = *p;
    e.coro_init();
    e.M1.Init(o.part);
    if (threads_failed(e.M2.Init(o.threads, MT_RING, &a, 1), o.threads))
      return E_INTERNAL;  // (started threads wait: they end with the process)
    mt_head(head, o.threads, o.part);
    s.write(head, sizeof head);
    y = run(e, mt_counts<MtEncWorkers, 0>, &e.M2, [&](uint) { pr.poll(); });
    if (!(r = mt_failed(e.M2, note, sizeof note)) && y != Y_END) r = E_INTERNAL;
    memset(&st, 0, sizeof st);
    memset(cost, 0, NCAT * sizeof *cost);
    memset(orig, 0, NCAT * sizeof *orig);
    for (i = 0; i < e.M2.th_N; i++) {
      const M4vPart<0>& w = e.M2.C1[i];
      st.add(w.st);
      for (j = 0; j < NCAT; j++) cost[j] += w.cost[j], orig[j] += w.orig[j];
    }
    st.in_bytes = (double)e.M1.total;  // (the parts have the headers copied in)
    st.parts = e.M1.parts;
    st.threads = o.threads;
    mt_sums(e.M2, n.raw, n.packed, n.vops);
    n.raw = e.M1.total;
    n.packed = s.pos;
    e.M2.Quit();  // (the threads joined, the workers freed)
    e.M1.Quit();
    coro_free(p);
    return run_end(r, note);
  }
};
