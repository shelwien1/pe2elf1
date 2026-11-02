// aacpack on threads (c with -t N, and d of what it wrote), as m4vpack's
// mt.inc (h264p's before it): the stream cut into parts by aacsplit.inc's
// splitter (AacSplit, on the main thread) -- a part ends before the first
// unit at least -c's part size after its start that parses the same at the
// start of a stream (else at the first unit past mt_cap(), or in bytes at
// twice that), so that each is a stream by itself -- and the parts dealt in
// turn to N workers by MTwrap (MTwrap_v3, its parts mode: MTwrap.md section
// 11). A worker codes its parts one after the other, each by a model of its
// own: AacpCoro (coro.inc), run as a coroutine of the worker's -- c on the
// part read whole into memory, its output written as it comes; d on the
// worker's input where it is, its output in memory until the part is done
// (the parts go out in turn). A worker's stream is a record for each part:
//
//   u64 plen    the size of the part that was compressed (with the bytes
//               the splitter put in; never 0)
//   u64 ins     where in it those are: [ins, ins + xlen), taken out after
//   u64 xlen    decoding (ADIF's or AUS1's header: 0 and its size; none:
//               0, 0)
//   u64 part    its place in the stream, from 0: worker w's k-th, w + kN
//   u32 crc     the CRC-32 of these 32 bytes; u32 0
//   then the part compressed, a stream of one model (magic, version,
//   level, order, ...), which ends where its decoder stops reading (rc.inc's
//   decoder reads exactly the bytes its coder wrote): the next record
//   follows it
//
// and its end, a record of plen 0 (ins and xlen 0) alone, whose part is the
// place the part it would have had next has (little-endian). So d finds a
// part lost, added or moved, the end of a worker's stream cut off, or a
// record broken, where each part's own CRC (its stream's) cannot. A worker
// writes its stream as it codes it, 64 KB at a time (and the rest of a part
// as it ends), and MTwrap puts each block in its container as it comes: so
// the container has the workers' streams interleaved as they were made, and
// d, whose workers decode about as fast as c's coded, finds each worker's
// input about when it needs it. (Version 8 wrote a part's record when the
// part was done, with the size of its stream: d then took the container
// about a part at a time while the other workers waited for theirs.)
// MTwrap's container holds the N streams; aacpack's header goes before it
// (kMtMagic, mt_head()). Decoding needs the same N (the header has it);
// each part ends a round of MTwrap's decoder (yield 4), which takes the
// parts from the workers in turn as they were dealt.

namespace aacpack {

const char kMtMagic[] = "aacpm";  // compressed on threads, followed by the version, the threads and the part size in KB
const int kMtHead = (int)sizeof kMtMagic + 1 + 4;  // the magic, the version, the threads, the part size (u32)
// MTwrap's rings between the main thread and a worker: their size does not
// depend on the parts (a part goes through them a buffer at a time)
const uint32_t kMtRing = 1 << 20;
// A worker's record (above): its fields, the CRC-32 of the first 32 bytes
const int kMtRec = 40;
// The splitter's frame of a part: L, ins, xlen, at and xat (aacsplit.inc)
const int kMtFrame = 40;
uint32_t mt_rec_crc(const uint8_t* h) {
  Crc32 c;
  c.Init();
  c.add(h, 32);
  return c.value();
}
void mt_rec(uint8_t* h, uint64_t plen, uint64_t ins, uint64_t xlen, uint64_t part) {
  put_le(h, plen, 8);
  put_le(h + 8, ins, 8);
  put_le(h + 16, xlen, 8);
  put_le(h + 24, part, 8);
  put_le(h + 32, mt_rec_crc(h), 4);
  put_le(h + 36, 0, 4);
}

// aacpack's header of a stream compressed on threads (the part size bounds
// what d takes a part's record to be: mt_part_max())
void mt_head(uint8_t* head, int threads, uint32_t part) {
  memcpy(head, kMtMagic, sizeof kMtMagic - 1);
  head[sizeof kMtMagic - 1] = (uint8_t)kCmVersion;
  head[sizeof kMtMagic] = (uint8_t)threads;
  put_le(head + sizeof kMtMagic + 1, part >> 10, 4);
}
// The size past which a part ends at any frame (with none that parses the
// same at a stream's start), for the memory a part takes
uint64_t mt_cap(uint64_t part) { return Min<uint64_t>(4 * part, (uint64_t)1 << 31); }
// The most a part can be, as the splitter cuts it: twice the cap, past
// which it cuts inside bytes, or the cap and a unit that goes on past it (a
// raw_data_block of ADIF or -asc, at most kBlockMax; an access unit of
// AUS1 with its length, kAuMax + 4, which is less than the cap, at least
// 4 KB, and kBlockMax), and the header of ADIF or AUS1 put in before it
// (at most AUS1's 6 + 65535 bytes). d takes a record of a larger part, or
// one whose part comes out larger than it says, to be broken: so a part's
// output it holds is bounded by -c too.
uint64_t mt_part_max(uint64_t part) { return 2 * mt_cap(part) + kBlockMax + 6 + 0xFFFF; }

// A worker's counts for the progress line: its input, its output and its
// frames so far (their parts' and the part's under way), read from any
// thread
struct PartProg {
  uint64_t in_, out_, frames_;
  void set(uint64_t i, uint64_t o, uint64_t f) {
    __atomic_store_n(&in_, i, __ATOMIC_RELAXED);
    __atomic_store_n(&out_, o, __ATOMIC_RELAXED);
    __atomic_store_n(&frames_, f, __ATOMIC_RELAXED);
  }
  uint64_t in() const { return __atomic_load_n(&in_, __ATOMIC_RELAXED); }
  uint64_t out() const { return __atomic_load_n(&out_, __ATOMIC_RELAXED); }
  uint64_t frames() const { return __atomic_load_n(&frames_, __ATOMIC_RELAXED); }
};

// MTwrap's Enc_class: a worker. Plain data, never constructed (MTwrap maps
// its workers): Init() sets it up, do_process() allocates what it needs,
// and frees it as it ends, or Quit() wherever it was left.
template <int f_DEC>
struct AacpPart : CoroIo {
  AacArgs args;       // the call's (its stats: the workers' statistics, one each, -v)
  Buf<uint8_t> in_;   // compressing, a part, all of it, read before its model starts
  Buf<uint8_t> out_;  // compressing, the part's output a buffer at a time; decompressing, all of it
  uint64_t parts;     // the parts done (from another thread: done())
  PartProg prog;      // the input coded so far and the output (the progress line)
  uint32_t worker;    // its place among the workers (MTwrap's), dealt the parts in turn

  uint32_t Init(const AacArgs* a, uint32_t w) {
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
    say_error(args.meter, "aacpack: %s: %s\n", args.name, why);
    return 1;
  }
  // the next n bytes of the input into in_ (false: there are fewer); in_
  // grown as they come, not by n, which a broken frame can make anything
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

  // d: a part's run reads the worker's input where it is: lend() hands it
  // the rest of the input buffer (p, its size; 0: the input has ended) as
  // if it were read, and take_back() takes back what the run left of it
  // (past the part's stream: the next record), where the run's last input
  // was that buffer (from p to e)
  size_t lend(const uint8_t*& p) {
    if (inpptr >= inpend && !next_input()) return 0;
    p = (const uint8_t*)inpptr;
    size_t l = (size_t)(inpend - inpptr);
    inpptr = inpend;
    return l;
  }
  void take_back(byte* p, const byte* e) {
    if (e == inpend && p <= inpend) inpptr = p;
  }

  // A part's run of a codec, C (AacpCoro<f_DEC, LV, CS>), on what src gives
  // it (src(p): the next piece at p, its size; 0, the end): c, its output
  // into the worker's output as it comes (MTwrap puts each block of the
  // worker's in its container); d, into out_ (grown as it fills, to most
  // bytes: one more fails the worker), and what it leaves of its input
  // taken back. The run's status (0, 1 after an error). As it goes, its
  // input, output and frames from in0, out0 and frames0 on are the
  // worker's counts, what it tells is shown (show_told(), its statistics
  // added to the worker's), and a line of progress may be due.
  template <class C, class Src>
  int run_part(const AacArgs& a, Src& src, uint64_t most, uint64_t in0, uint64_t out0, uint64_t frames0) {
    const size_t kOut = (size_t)1 << 16;
    C* c = coro_new<C>();
    c->Init(&a);
    auto feed = [&]() {  // the next piece of the part (addinp() takes 32 bits), or its end
      const uint8_t* p = nullptr;
      size_t l = src(p);
      if (!l) c->f_quit = 1;
      c->addinp((byte*)(l ? p : out_.data()), (uint)l);
    };
    out_.resize(f_DEC ? (size_t)Min<uint64_t>(kOut, most + 1) : kOut);
    c->addout(out_.data(), (uint)out_.size());
    feed();
    drive(
        *c, feed,
        [&](bool more) {
          size_t done = (size_t)((uint8_t*)c->outptr - out_.data());
          if (!f_DEC) {  // (c: on to the container as it comes)
            write(out_.data(), done);
            if (more) c->addout(out_.data(), (uint)kOut);
            return;
          }
          if (!more) {  // the end, or an error
            out_.resize(done);
            return;
          }
          if (done == out_.size()) {  // (the output full: twice the room, to one byte past the most)
            if (done > most) err_raise_s("a part came out longer than its record says");
            out_.resize((size_t)Min<uint64_t>(2 * (uint64_t)done, most + 1));
          }
          c->addout(out_.data() + done, (uint)Min(out_.size() - done, kOut));
        },
        [&](uint code) {
          show_told(*c, code, a.meter, a);
          prog.set(in0 + c->n_in, out0 + c->n_out, frames0 + c->frames);
          if (a.meter) a.meter->poll();
        });
    if (f_DEC) take_back(c->inpptr, c->inpend);
    if (a.stats && c->depth > a.stats->depth) a.stats->depth = c->depth;
    int r = c->status;
    c->Quit();
    coro_free(c);
    return r;
  }
  // the part's model run on what src gives (run_part()), at the level and
  // in the order of a (the counts from in0, out0 and frames0 on)
  template <class LV>
  struct Lv {
    template <class Src>
    static int run(AacpPart& w, const AacArgs& a, Src& src, uint64_t most, uint64_t in0, uint64_t out0,
                   uint64_t frames0) {
      return a.order ? w.template run_part<AacpCoro<f_DEC, LV, 1>>(a, src, most, in0, out0, frames0)
                     : w.template run_part<AacpCoro<f_DEC, LV, 0>>(a, src, most, in0, out0, frames0);
    }
  };
  template <class Src>
  int code(const AacArgs& a, Src& src, uint64_t most, uint64_t in0, uint64_t out0, uint64_t frames0) {
    int r = with_level<Lv>(a.level, *this, a, src, most, in0, out0, frames0);
    if (r != kNoLevel) return r;  // (0, or 1)
    say_error(args.meter, "aacpack: %s: a part compressed at level -%d, which this build has not\n", args.name,
              a.level);
    return 1;
  }

  // the place of the next part it has (worker w's k-th: w + kN)
  uint64_t next_part() const { return worker + parts * (uint64_t)args.threads; }
  // a part done; and the parts done, from any thread
  void part_done() { __atomic_store_n(&parts, parts + 1, __ATOMIC_RELAXED); }
  uint64_t done() const { return __atomic_load_n(&parts, __ATOMIC_RELAXED); }
  // the arguments of a part's run: the call's, with the worker's statistics
  AacArgs part_args() const {
    AacArgs a = args;
    if (a.stats) a.stats += worker;
    return a;
  }

  // compressing: each part as the splitter frames it (u64 L, then u64 ins,
  // xlen, at and xat, then the part, L - 32 bytes) into its record and the
  // part compressed, written as it comes; and as the parts end, the record
  // of the worker's end
  int run(IntTag<0>) {
    for (;;) {
      uint8_t h[kMtFrame], rec[kMtRec];
      size_t n = read(h, kMtFrame);
      if (!n) {
        mt_rec(rec, 0, 0, 0, next_part());
        write(rec, kMtRec);
        return 0;
      }
      uint64_t L = get_le(h, 8), ins = get_le(h + 8, 8), xlen = get_le(h + 16, 8);
      uint64_t plen = L - (kMtFrame - 8);
      if (n < (size_t)kMtFrame || L <= (uint64_t)kMtFrame - 8 || ins > plen || xlen > plen - ins || !take(plen))
        return fail("internal: a part framed wrong");  // (plen 0 is the worker's end)
      AacArgs a = part_args();  // (with where the part is in the input, for messages)
      a.at = get_le(h + 24, 8), a.ins = ins, a.xlen = xlen, a.xat = get_le(h + 32, 8);
      mt_rec(rec, plen, ins, xlen, next_part());
      write(rec, kMtRec);
      uint64_t in0 = prog.in(), out0 = prog.out() + kMtRec, frames0 = prog.frames();
      const uint64_t kAny = ~(uint64_t)0 >> 1;  // (a part's compressed size: as it comes)
      size_t pos = 0;
      auto src = [&](const uint8_t*& p) {  // in_, from pos on
        size_t l = Min(in_.size() - pos, (size_t)1 << 30);
        p = in_.data() + pos;
        pos += l;
        return l;
      };
      if (code(a, src, kAny, in0, out0, frames0)) return 1;
      prog.set(in0 + plen - xlen, prog.out(), prog.frames());  // (the bytes put in are not the stream's)
      part_done();
      // the part's last buffer handed over now, as far as it is filled (a
      // round's end, as d's worker ends one): so the part's end is in the
      // container as it is done, not after some of the next part
      before_yield();
      yield(static_cast<Coroutine*>(this), 4);
    }
  }

  // decompressing: each record into its part, the bytes the splitter put
  // in taken out, at the end of which MTwrap takes the next part from the
  // next worker; to the record of the worker's end, after which nothing
  int run(IntTag<1>) {
    for (;;) {
      uint8_t h[kMtRec];
      if (read(h, kMtRec) < (size_t)kMtRec) return fail("a broken or truncated stream: a worker's records cut short");
      uint64_t plen = get_le(h, 8), ins = get_le(h + 8, 8), xlen = get_le(h + 16, 8);
      if (get_le(h + 32, 4) != mt_rec_crc(h) || get_le(h + 36, 4) || get_le(h + 24, 8) != next_part())
        return fail("a part's record broken, or a part lost, added or moved");
      if (!plen) {  // the worker's end
        uint8_t x;
        if (ins || xlen) return fail("a part's record broken");
        if (read(&x, 1)) return fail("data after a worker's end");
        return 0;
      }
      if (plen > args.part_max || ins > plen || xlen > plen - ins) return fail("a part's record broken");
      // the part compressed: its header here (its level and order), the
      // rest read by its run where it is in the worker's input (lend())
      uint8_t head[sizeof kCmMagic + 2];
      if (read(head, sizeof head) < sizeof head) return fail("a broken or truncated stream: a part cut short");
      AacArgs a = part_args();
      a.part = true;
      a.level = cm_level(head, sizeof head, args.name, note, sizeof note, &a.order);
      if (a.level < 0) {
        say_error(args.meter, "%s", note);
        return 1;
      }
      bool first = true;
      auto src = [&](const uint8_t*& p) -> size_t {
        if (first) {
          first = false;
          p = head;
          return sizeof head;
        }
        return lend(p);
      };
      uint64_t in0 = prog.in() + kMtRec, out0 = prog.out(), frames0 = prog.frames();
      if (code(a, src, plen, in0, out0, frames0)) return 1;
      if (out_.size() != plen) return fail("a part did not decompress as it was compressed");
      size_t pos = (size_t)(ins + xlen);
      prog.set(prog.in(), out0 + plen - xlen, prog.frames());
      write(out_.data(), (size_t)ins);
      write(out_.data() + pos, out_.size() - pos);
      part_done();
      before_yield();
      yield(static_cast<Coroutine*>(this), 4);
    }
  }
};

// c: the splitter into MTwrap's coder; d: MTwrap's decoder
typedef CoroutinePair<AacSplit, MTwrap<0, AacpPart<0>, const AacArgs*>> MtEnc;
typedef MTwrap<1, AacpPart<1>, const AacArgs*> MtDec;

// What the workers did: the input they have coded so far and their output
// (the progress line; from any thread), their parts, and whether one
// failed (and said why)
struct MtWork {
  uint64_t in, out, frames, parts;
  bool failed;
};
template <class M>
MtWork mt_work(const M& m) {
  MtWork w = {0, 0, 0, 0, false};
  for (uint i = 0; i < m.th_N; i++) {
    w.in += m.C1[i].prog.in();
    w.out += m.C1[i].prog.out();
    w.frames += m.C1[i].prog.frames();
    w.parts += m.C1[i].done();
    w.failed = w.failed || m.C1[i].f_fail;
  }
  return w;
}
template <class M>
bool mt_failed(const M& m) {
  return mt_work(m).failed;
}
// What a run on threads did, as aacpack reports it (-v, the progress
// line's end): the stream's bytes (raw), the compressed stream's with
// aacpack's header (packed), the frames and the parts. (Here, as the rest
// of MTwrap's and the splitter's insides aacpack reads, so that the calls
// see none of them.)
struct MtCounts {
  uint64_t raw, packed, frames, parts;
};
MtCounts mt_counts(const MtEnc& e) {
  return {e.M1.st->total, kMtHead + e.M2.c_out, mt_work(e.M2).frames, e.M1.st->parts};
}
MtCounts mt_counts(const MtDec& d) { return {d.c_out, kMtHead + d.c_inp, mt_work(d).frames, mt_work(d).parts}; }
// whether the workers had the parts as they are dealt, in turn: of P in
// all, worker i the i-th of each round of N, one more where P % N is past
// it. (Each worker's records say their places, to its end's.)
bool mt_dealt(const MtDec& d) {
  uint64_t P = mt_work(d).parts, N = d.th_N;
  for (uint i = 0; i < d.th_N; i++)
    if (d.C1[i].done() != P / N + (i < P % N)) return false;
  return true;
}
}  // namespace aacpack
