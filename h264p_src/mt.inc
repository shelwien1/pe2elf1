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
//   u64 g_size  the record's size, these kMtRec (48) bytes included
//   u64 plen    the size of the part as compressed (with the parameter
//               sets the splitter inserted)
//   u64 ins     where in it those are: [ins, ins + xlen), removed after decoding
//   u64 xlen
//   u64 part    its place in the stream, from 0: worker w's k-th, w + kN
//   u32 crc     the CRC-32 of these 40 bytes; u32 0
//   g_size-48 bytes: the part compressed, a stream of h264p's (magic, version, level, ...)
//
// and its end, a record of the 48 bytes alone, whose part is the place of
// the part it would have had next (little-endian). So d finds a part lost,
// added or moved, the end of a worker's stream cut off, or a record broken,
// where each part's own CRC (its stream's) cannot: c86afa6 ended each part
// with a block of its own, so that whole parts could go, or a stream end
// after one, as whole blocks. MTwrap's container holds the N streams;
// h264p's header goes before it (kMtMagic, mt_head()). Decoding needs the
// same N (the header has it); each part ends a round of MTwrap's decoder
// (yield 4), which takes the parts from the workers in turn as they were
// dealt.

namespace h264p {

const char kMtMagic[] = "h264m";  // compressed on threads, followed by the version, the threads and the part size in KB
const int kMtHead = (int)sizeof kMtMagic + 1 + 4;  // the magic, the version, the threads, the part size (u32)
// MTwrap's rings between the main thread and a worker: their size does not
// depend on the parts (a part goes through them a buffer at a time)
const uint32_t kMtRing = 1 << 20;
// A worker's record (above): its fields, the CRC-32 of the first 40 bytes
const int kMtRec = 48;
uint32_t mt_rec_crc(const uint8_t* h) {
  Crc32 c;
  c.Init();
  c.add(h, 40);
  return c.value();
}

void put_le(uint8_t* p, uint64_t v, int n) {
  for (int i = 0; i < n; i++) p[i] = (uint8_t)(v >> 8 * i);
}
uint64_t get_le(const uint8_t* p, int n) {
  uint64_t v = 0;
  for (int i = 0; i < n; i++) v |= (uint64_t)p[i] << 8 * i;
  return v;
}
void mt_rec(uint8_t* h, uint64_t g_size, uint64_t plen, uint64_t ins, uint64_t xlen, uint64_t part) {
  put_le(h, g_size, 8);
  put_le(h + 8, plen, 8);
  put_le(h + 16, ins, 8);
  put_le(h + 24, xlen, 8);
  put_le(h + 32, part, 8);
  put_le(h + 40, mt_rec_crc(h), 4);
  put_le(h + 44, 0, 4);
}

// h264p's header of a stream compressed on threads (the part size bounds
// what d takes a part's record to be: mt_part_max())
void mt_head(uint8_t* head, int threads, uint32_t part) {
  memcpy(head, kMtMagic, sizeof kMtMagic - 1);
  head[sizeof kMtMagic - 1] = (uint8_t)kCmVersion;
  head[sizeof kMtMagic] = (uint8_t)threads;
  put_le(head + sizeof kMtMagic + 1, part >> 10, 4);
}
// The size past which a part ends at any picture (with no IDR or I access
// unit to cut at), for the memory a part takes
uint64_t mt_cap(uint64_t part) { return Min<uint64_t>(4 * part, (uint64_t)1 << 31); }
// The most a part can be, as the splitter cuts it: twice the cap, past which
// it cuts after each piece of the input (64 KB), and the parameter sets it
// copies in (kSlots of at most kMaxPset bytes, after start codes). d takes
// a record of a larger part, or one whose part comes out larger than it
// says, to be broken: so a part's output it holds is bounded by -c too.
uint64_t mt_part_max(uint64_t part) {
  return 2 * mt_cap(part) + (1 << 20) + (uint64_t)h264s::kSlots * (h264s::kMaxPset + 4);
}

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

// A run of a codec, H264pCoro<f_DEC, LV>, on a part: its input as src
// gives it (src(p): the next piece at p, its size; 0, the end), its output
// into out (grown as it fills, handed to it 64 KB at a time, to most bytes:
// one more fails the worker); the run's status (0, 1 after an error,
// kRestart). As it goes,
// its input, output and pictures from in0, out0 and pics0 on are the
// worker's counts, what it tells is shown (show_told(): the parts' video
// once), and a line of progress may be due.
template <class C, class Src>
int run_part(const H264pArgs& a, Src& src, Buf<uint8_t>& out, uint64_t most, PartProg& prog, uint64_t in0,
             uint64_t out0, uint64_t pics0) {
  C* c = coro_new<C>();
  c->Init(&a);
  auto feed = [&]() {  // the next piece of the part (addinp() takes 32 bits), or its end
    const uint8_t* p = nullptr;
    size_t l = src(p);
    if (!l) c->f_quit = 1;
    c->addinp((byte*)(l ? p : out.data()), (uint)l);
  };
  out.resize((size_t)Min<uint64_t>(1 << 16, most + 1));
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
        if (done == out.size()) {  // (the output full: twice the room, to one byte past the most)
          if (done > most) err_raise("a part came out longer than its record says");
          out.resize((size_t)Min<uint64_t>(2 * (uint64_t)done, most + 1));
        }
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
  Buf<uint8_t> in_;   // compressing, a part, all of it, read before its model starts; decompressing, 64 KB of its record
  Buf<uint8_t> out_;  // what the part's model made of it
  uint64_t parts;              // the parts done
  PartProg prog;               // the input coded so far and the output (the progress line)
  uint32_t worker;             // its place among the workers (MTwrap's), dealt the parts in turn

  uint32_t Init(const H264pArgs* a, uint32_t w) {
    coro_init();
    args = *a;
    worker = w;
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
  // the part's model run on what src gives (run_part()) into out_ (most
  // bytes), at the level of a.level (the counts from in0, out0 and pics0 on)
  template <class LV>
  struct Lv {
    template <class Src>
    static int run(H264pPart& w, const H264pArgs& a, Src& src, uint64_t most, uint64_t in0, uint64_t out0,
                   uint64_t pics0) {
      return run_part<H264pCoro<f_DEC, LV>>(a, src, w.out_, most, w.prog, in0, out0, pics0);
    }
  };
  template <class Src>
  int code(const H264pArgs& a, Src& src, uint64_t most, uint64_t in0, uint64_t out0, uint64_t pics0) {
    int r = with_level<Lv>(a.level, *this, a, src, most, in0, out0, pics0);
    if (r != kNoLevel) return r;  // (0, 1, or kRestart)
    say_error(args.meter, "h264p: %s: a part compressed at level -%d, which this build has not\n", args.name, a.level);
    return 1;
  }

  // the place of the next part it has (worker w's k-th: w + kN)
  uint64_t next_part() const { return worker + parts * (uint64_t)args.threads; }

  // compressing: each part as the splitter frames it (u64 L, then u64 ins,
  // xlen, nins and nal0, then the part, L-32 bytes) into its record, and as
  // the parts end, the record of the worker's end
  int run(IntTag<0>) {
    for (;;) {
      uint8_t h[40], rec[kMtRec];
      size_t n = read(h, 40);
      if (!n) {
        mt_rec(rec, kMtRec, 0, 0, 0, next_part());
        write(rec, kMtRec);
        return 0;
      }
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
      const uint64_t kAny = ~(uint64_t)0 >> 1;  // (a part's compressed size: as it comes)
      size_t pos = 0;
      auto src = [&](const uint8_t*& p) {  // in_, from pos on (each run from its start)
        size_t l = Min(in_.size() - pos, (size_t)1 << 30);
        p = in_.data() + pos;
        pos += l;
        return l;
      };
      int r = code(a, src, kAny, in0, out0, pics0);
      if (r == kRestart) {
        a.check = false;
        pos = 0;
        r = code(a, src, kAny, in0, out0, pics0);
      }
      if (r) return 1;
      prog.set(in0 + plen, out0 + kMtRec + out_.size(), prog.pics());
      mt_rec(rec, out_.size() + kMtRec, plen, ins, xlen, next_part());
      write(rec, kMtRec);
      write(out_.data(), out_.size());
      parts++;
      // the record handed over now, its last buffer as far as it is filled
      // (a round's end, as d's worker ends one): so MTwrap puts each record
      // in its stream as its part is done, and d, which takes the parts in
      // turn, finds the next within about a round of the others. (A worker
      // whose records are small held them until 64 KB of them, thousands of
      // parts, past all of which d had to read the others' and hold them.)
      before_yield();
      yield(static_cast<Coroutine*>(this), 4);
    }
  }

  // decompressing: each record into its part, the inserted parameter sets
  // removed, at the end of which MTwrap takes the next part from the next
  // worker; to the record of the worker's end, after which nothing
  int run(IntTag<1>) {
    for (;;) {
      uint8_t h[kMtRec];
      if (read(h, kMtRec) < (size_t)kMtRec) return fail("a broken or truncated stream: a worker's records cut short");
      uint64_t g_size = get_le(h, 8), plen = get_le(h + 8, 8), ins = get_le(h + 16, 8), xlen = get_le(h + 24, 8);
      if (get_le(h + 40, 4) != mt_rec_crc(h) || get_le(h + 44, 4) || get_le(h + 32, 8) != next_part())
        return fail("a part's record broken, or a part lost, added or moved");
      if (g_size == (uint64_t)kMtRec) {  // the worker's end
        uint8_t x;
        if (plen || ins || xlen) return fail("a part's record broken");
        if (read(&x, 1)) return fail("data after a worker's end");
        return 0;
      }
      if (plen > args.part_max || ins > plen || xlen > plen - ins || g_size < (uint64_t)kMtRec)
        return fail("a part's record broken");
      // the part compressed, read as its model asks for it, none of it
      // held but 64 KB (whatever size the record says): its first bytes
      // here (its level), then the rest of g_size from the input
      uint64_t left = g_size - kMtRec;
      uint8_t head[sizeof kCmMagic + 1];
      size_t nh = (size_t)Min<uint64_t>(left, sizeof head);
      if (read(head, nh) < nh) return fail("a part's record cut short");
      left -= nh;
      H264pArgs a = args;
      a.level = cm_level(head, nh, args.name, note, sizeof note);
      if (a.level < 0) {
        say_error(args.meter, "%s", note);
        return 1;
      }
      in_.resize((size_t)1 << 16);
      bool first = true;
      auto src = [&](const uint8_t*& p) {
        if (first) {
          first = false;
          p = head;
          return nh;
        }
        size_t got = read(in_.data(), (size_t)Min<uint64_t>(left, in_.size()));
        left -= got;
        p = in_.data();
        return got;
      };
      uint64_t in0 = prog.in(), out0 = prog.out(), pics0 = prog.pics();
      if (code(a, src, plen, in0 + kMtRec, out0, pics0)) return 1;
      if (left) return fail("a part's record broken");  // (its stream ended before it)
      if (out_.size() != plen) return fail("a part did not decompress as it was compressed");
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
// whether the workers had the parts as they are dealt, in turn: of P in
// all, worker i the i-th of each round of N, one more where P % N is past
// it. (Each worker's records say their places, to its end's.)
bool mt_dealt(const MtDec& d) {
  uint64_t P = mt_work(d).parts, N = d.th_N;
  for (uint i = 0; i < d.th_N; i++)
    if (d.C1[i].parts != P / N + (i < P % N)) return false;
  return true;
}
}  // namespace h264p
