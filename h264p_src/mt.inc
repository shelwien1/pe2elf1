// h264p on threads (c with -t N, and d of what it wrote): the stream
// cut into parts by h264split.inc's splitter (H264Split, on the main
// thread) -- a part ends at the first IDR or I access unit at least -c's
// part size after its start, or at the first picture at least mt_cap()
// after it, and the parameter sets a part needs are copied in at its
// start, so that each is a stream by itself -- and the parts dealt in
// turn to N workers by MTwrap (MTwrap_v3, its parts mode: MTwrap.md section
// 11). A worker codes its parts one after the other, each by a model of its
// own: H264pCoro (coro.inc), run as a coroutine of the worker's on the part
// read whole into memory. A worker's stream is a record for each part:
//
//   u64 g_size  the record's size, these 32 bytes included
//   u64 plen    the size of the part as compressed (with the parameter
//               sets the splitter inserted)
//   u64 ins     where in it those are: [ins, ins + xlen), removed after decoding
//   u64 xlen
//   g_size-32 bytes: the part compressed, a stream of h264p's (magic, version, level, ...)
//
// (little-endian), and MTwrap's container holds the N streams; h264p's
// header goes before it (kMtMagic, mt_head()). Decoding needs the same N
// (the header has it); each part ends a round of MTwrap's decoder (yield 4),
// which takes the parts from the workers in turn as they were dealt.

namespace h264p {

const char kMtMagic[] = "h264m";  // compressed on threads, followed by the version, the threads and the part size in KB
const int kMtHead = (int)sizeof kMtMagic + 1 + 4;  // the magic, the version, the threads, the part size (u32)
// MTwrap's rings between the main thread and a worker: their size does not
// depend on the parts (a part goes through them a buffer at a time)
const uint32_t kMtRing = 1 << 20;

void put_le(uint8_t* p, uint64_t v, int n) {
  for (int i = 0; i < n; i++) p[i] = (uint8_t)(v >> 8 * i);
}
uint64_t get_le(const uint8_t* p, int n) {
  uint64_t v = 0;
  for (int i = 0; i < n; i++) v |= (uint64_t)p[i] << 8 * i;
  return v;
}

// h264p's header of a stream compressed on threads (the part size is
// there to be seen: d does not need it)
void mt_head(uint8_t* head, int threads, uint32_t part) {
  memcpy(head, kMtMagic, sizeof kMtMagic - 1);
  head[sizeof kMtMagic - 1] = (uint8_t)kCmVersion;
  head[sizeof kMtMagic] = (uint8_t)threads;
  put_le(head + sizeof kMtMagic + 1, part >> 10, 4);
}
// The size past which a part ends at any picture (with no IDR or I access
// unit to cut at), for the memory a part takes
uint64_t mt_cap(uint32_t part) { return Min<uint64_t>(4 * (uint64_t)part, (uint64_t)1 << 31); }

// A worker's counts for the progress line: its input, its output and its
// pictures so far (their parts' and the part's under way), read from any
// thread
struct PartProg {
  uint64_t in_, out_, pics_;
  void set(uint64_t i, uint64_t o, uint64_t p) {
    __atomic_store_n(&in_, i, __ATOMIC_RELAXED);
    __atomic_store_n(&out_, o, __ATOMIC_RELAXED);
    __atomic_store_n(&pics_, p, __ATOMIC_RELAXED);
  }
  uint64_t in() const { return __atomic_load_n(&in_, __ATOMIC_RELAXED); }
  uint64_t out() const { return __atomic_load_n(&out_, __ATOMIC_RELAXED); }
  uint64_t pics() const { return __atomic_load_n(&pics_, __ATOMIC_RELAXED); }
};

// A run of a codec, H264pCoro<f_DEC, LV>, on a part in memory: all of in
// its input, its output into out (grown as it fills, handed to it 64 KB at
// a time); the run's status (0, 1 after an error, kRestart). As it goes,
// its input, output and pictures from in0, out0 and pics0 on are the
// worker's counts, what it tells is shown (show_told(): the parts' video
// once), and a line of progress may be due.
template <class C>
int run_part(const H264pArgs& a, Buf<uint8_t>& in, Buf<uint8_t>& out, PartProg& prog, uint64_t in0, uint64_t out0,
             uint64_t pics0) {
  C* c = coro_new<C>();
  c->Init(&a);
  size_t pos = 0;
  auto feed = [&]() {  // the next piece of the part (addinp() takes 32 bits), or its end
    size_t l = Min(in.size() - pos, (size_t)1 << 30);
    if (!l) c->f_quit = 1;
    c->addinp(in.data() + pos, (uint)l);
    pos += l;
  };
  out.resize(1 << 16);
  c->addout(out.data(), (uint)out.size());
  feed();
  drive(
      *c, feed,
      [&](bool more) {
        size_t done = (size_t)(c->outptr - out.data());
        if (!more) {  // the end, or an error
          out.resize(done);
          return;
        }
        if (done == out.size()) out.resize(done * 2);  // (the output full: twice the room)
        c->addout(out.data() + done, (uint)Min(out.size() - done, (size_t)1 << 16));
      },
      [&](uint code) {
        show_told(*c, code, a.meter, a);
        prog.set(in0 + c->n_in, out0 + c->n_out, pics0 + c->pictures);
        progress_poll(a.meter);
      });
  int r = c->status;
  c->Quit();
  coro_free(c);
  return r;
}

// MTwrap's Enc_class: a worker. Plain data, never constructed (MTwrap maps
// its workers): Init() sets it up, do_process() allocates what it needs,
// and frees it as it ends, or Quit() wherever it was left.
template <int f_DEC>
struct H264pPart : CoroIo {
  H264pArgs args;
  Buf<uint8_t> in_;   // a part, or its record: all of it, read before its model starts
  Buf<uint8_t> out_;  // what the part's model made of it
  uint64_t parts;              // the parts done
  PartProg prog;               // the input coded so far and the output (the progress line)

  uint32_t Init(const H264pArgs* a) {
    coro_init();
    args = *a;
    in_.Init();
    out_.Init();
    parts = 0;
    prog.set(0, 0, 0);
    return 0;
  }
  void Quit() {
    in_.Quit();
    out_.Quit();
  }

  // (an error in a part's model ends that run at its own trap: run_part()
  // returns its status. One outside it, as no memory for the part, its
  // model or its output, fails the worker here, at the worker's trap: the
  // call fails, and a part's model left at that is freed as it ends.)
  void do_process() {
    io_init();
    int r;
    ErrTrap t;
    if (!setjmp(t.jb)) {
      err_push(t);
      r = run(IntTag<f_DEC>());
      err_pop(t);
    } else {
      r = fail(t.why);
    }
    Quit();
    for (;;) yield(static_cast<Coroutine*>(this), r ? 3 : 0);
  }

  int fail(const char* why) {
    say_error(args.meter, "h264p: %s: %s\n", args.name, why);
    return 1;
  }
  // the next n bytes of the input into in_ (false: there are fewer); in_
  // grown as they come, not by n, which a broken record can make anything
  bool take(uint64_t n) {
    if (n != (size_t)n) return false;
    in_.clear();
    while (in_.size() < n) {
      size_t m = in_.size(), k = (size_t)Min<uint64_t>(n - m, (uint64_t)1 << 20);
      in_.resize(m + k);
      size_t got = read(in_.data() + m, k);
      in_.resize(m + got);
      if (got < k) return false;
    }
    return true;
  }
  // the part's model run on in_ into out_, at the level of a.level (the
  // counts from in0, out0 and pics0 on)
  template <class LV>
  struct Lv {
    static int run(H264pPart& w, const H264pArgs& a, uint64_t in0, uint64_t out0, uint64_t pics0) {
      return run_part<H264pCoro<f_DEC, LV>>(a, w.in_, w.out_, w.prog, in0, out0, pics0);
    }
  };
  int code(const H264pArgs& a, uint64_t in0, uint64_t out0, uint64_t pics0) {
    int r = with_level<Lv>(a.level, *this, a, in0, out0, pics0);
    if (r != kNoLevel) return r;  // (0, 1, or kRestart)
    say_error(args.meter, "h264p: %s: a part compressed at level -%d, which this build has not\n", args.name, a.level);
    return 1;
  }

  // compressing: each part as the splitter frames it (u64 L, then u64 ins,
  // xlen, nins and nal0, then the part, L-32 bytes) into its record
  int run(IntTag<0>) {
    for (;;) {
      uint8_t h[40];
      size_t n = read(h, 40);
      if (!n) return 0;  // the end
      uint64_t L = get_le(h, 8), ins = get_le(h + 8, 8), xlen = get_le(h + 16, 8);
      uint64_t plen = L - 32;
      if (n < 40 || L < 32 || ins > plen || xlen > plen - ins || !take(plen))
        return fail("internal: a part framed wrong");
      // (the part can be read again: the model pass checks the slices it
      // can, and if one does not come out as it is, the dry runs write back
      // each, again)
      H264pArgs a = args;
      a.check = true;
      a.nal0 = get_le(h + 32, 8), a.ncopy = get_le(h + 24, 8), a.copy_at = ins > 0;  // (after an AUD, if any)
      uint64_t in0 = prog.in(), out0 = prog.out(), pics0 = prog.pics();
      int r = code(a, in0, out0, pics0);
      if (r == kRestart) {
        a.check = false;
        r = code(a, in0, out0, pics0);
      }
      if (r) return 1;
      prog.set(in0 + plen, out0 + 32 + out_.size(), prog.pics());
      put_le(h, out_.size() + 32, 8);
      put_le(h + 8, plen, 8);
      put_le(h + 16, ins, 8);
      put_le(h + 24, xlen, 8);
      write(h, 32);
      write(out_.data(), out_.size());
      parts++;
    }
  }

  // decompressing: each record into its part, the inserted parameter sets
  // removed, at the end of which MTwrap takes the next part from the next
  // worker
  int run(IntTag<1>) {
    for (;;) {
      uint8_t h[32];
      size_t n = read(h, 32);
      if (!n) return 0;  // the end
      if (n < 32) return fail("a part's record cut short");
      uint64_t g_size = get_le(h, 8), plen = get_le(h + 8, 8), ins = get_le(h + 16, 8), xlen = get_le(h + 24, 8);
      if (g_size < 32 || !take(g_size - 32)) return fail("a part's record cut short or broken");
      H264pArgs a = args;
      a.level = cm_level(in_.data(), Min(in_.size(), sizeof kCmMagic + 1), args.name, note, sizeof note);
      if (a.level < 0) {
        say_error(args.meter, "%s", note);
        return 1;
      }
      uint64_t in0 = prog.in(), out0 = prog.out(), pics0 = prog.pics();
      if (code(a, in0 + 32, out0, pics0)) return 1;
      if (out_.size() != plen || ins > plen || xlen > plen - ins)
        return fail("a part did not decompress as it was compressed");
      size_t pos = (size_t)(ins + xlen);
      prog.set(in0 + g_size, out0 + plen - xlen, prog.pics());
      write(out_.data(), (size_t)ins);
      write(out_.data() + pos, out_.size() - pos);
      parts++;
      before_yield();
      yield(static_cast<Coroutine*>(this), 4);
    }
  }
};

// c: the splitter into MTwrap's coder; d: MTwrap's decoder
typedef CoroutinePair<H264Split, MTwrap<0, H264pPart<0>, const H264pArgs*>> MtEnc;
typedef MTwrap<1, H264pPart<1>, const H264pArgs*> MtDec;

// What the workers did: the input they have coded so far and their output
// (the progress line; from any thread), their parts, and whether one
// failed (and said why)
struct MtWork {
  uint64_t in, out, pics, parts;
  bool failed;
};
template <class M>
MtWork mt_work(const M& m) {
  MtWork w = {0, 0, 0, 0, false};
  for (uint i = 0; i < m.th_N; i++) {
    w.in += m.C1[i].prog.in();
    w.out += m.C1[i].prog.out();
    w.pics += m.C1[i].prog.pics();
    w.parts += m.C1[i].parts;
    w.failed = w.failed || m.C1[i].f_fail;
  }
  return w;
}
template <class M>
bool mt_failed(const M& m) {
  return mt_work(m).failed;
}
// What a run on threads did, as h264p reports it (-v, the progress line's
// end): the stream's bytes (raw), the compressed stream's with h264p's
// header (packed), and the parts. (Here, as the rest of MTwrap's and the
// splitter's insides h264p reads, so that the calls see none of them.)
struct MtCounts {
  uint64_t raw, packed, pics, parts;
};
MtCounts mt_counts(const MtEnc& e) {
  return {e.M1.st->total, kMtHead + e.M2.c_out, mt_work(e.M2).pics, e.M1.st->parts};
}
MtCounts mt_counts(const MtDec& d) { return {d.c_out, kMtHead + d.c_inp, mt_work(d).pics, mt_work(d).parts}; }
}  // namespace h264p
