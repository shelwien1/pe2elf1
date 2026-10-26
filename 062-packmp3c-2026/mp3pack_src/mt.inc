// mp3pack on threads (c with -t N, and d of what it wrote), as aacpack's
// mt.inc: the splitter (split.inc, on the main thread) puts the frames'
// records out in parts, and MTwrap (MTwrap_v3, its parts mode: MTwrap.md
// section 11) deals the parts in turn to N workers. A worker reads a part
// whole and codes it with its model -- mp3c's (mp3mod), run as a coroutine
// of the worker's -- then the next part dealt to it. The model goes on from
// one of the worker's parts to the next (its range coder starts again for
// each): d deals the parts to the workers in the same turns, so a worker
// decodes the parts it coded, in that order, and its model learns from all
// of them (aacpack's starts from nothing in each part). A worker's stream
// is a record for each part:
//
//   u64 csize  the part compressed: its bytes after the record
//   u64 dsize  the part's records, as the model gives them back
//   u64 part   its place in the file: worker w's k-th of N, w + kN
//   csize bytes: the model's stream
//
// and its end, a record of csize and dsize 0 whose part is the place of the
// part it would have had next (little-endian). So d finds a part lost,
// added or moved, and the end of a worker's stream cut off. MTwrap's
// container holds the N streams; d takes the parts from the workers in
// turn, as they were dealt (each part ends a round of MTwrap's decoder,
// yield 4), into the joiner on the main thread (join.inc), which reads the
// outside data. Decoding needs the same N: the header has it.

namespace mp3pack {

const int kMtRec = 24;
const int kPartHead = 32;  // the splitter's frame of a part: L, at, raw, frames (split.inc)
// MTwrap's rings between the main thread and a worker: their size does not
// depend on the parts (a part goes through them a buffer at a time)
const uint32_t kMtRing = 1 << 20;
// the 0xFF bytes past a model's stream (a range coder reads at most 4
// bytes past what its encoder flushed): one that wants more is broken
const uint8_t kPad[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// What the workers are given
struct PackArgs {
  Progress* meter;    // messages, and the progress line
  const char* name;   // the input's (messages)
  int threads;
  uint64_t dump_max;  // d: a part's records at most (from the part size)
};

// a part's records at most, for the part size part (split.inc: a part ends
// before the record past kDumpCap times it, and its end's record)
uint64_t dump_max(uint64_t part) { return kDumpCap * part + 2 * (sizeof(mp3frame0) + 32); }

// A worker's counts for the progress line: the input it stands for, its
// output and its frames so far, read from any thread
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

// The model on a part: m (mp3mod, set up, and as the parts before it left
// it) codes what feed() gives it (its addinp(); false: there is no more, and the
// model wants more: a broken part) into out, grown as it fills, to most
// bytes (one more: false); tick(), as it yields, with its output so far.
// false: the part is broken.
template <class M, class Feed, class Tick>
bool run_model(M* m, Feed feed, Vec<uint8_t>& out, uint64_t most, Tick tick) {
  m->coro_init();
  out.resize((size_t)Min<uint64_t>(1 << 16, most + 1));
  m->addout(out.data(), (uint)out.size());
  for (;;) {
    uint r = m->coro_call(m);
    size_t done = (size_t)(m->outptr - out.data());
    if (r == 1) {
      if (!feed()) return false;
    } else if (r == 2) {
      if (done == out.size()) {  // (full: twice the room, to one byte past the most)
        if (done > most) return false;
        out.resize((size_t)Min<uint64_t>(2 * (uint64_t)done, most + 1));
      }
      m->addout(out.data() + done, (uint)Min<size_t>(out.size() - done, (size_t)1 << 16));
    } else {
      out.resize(done);
      return r == 0;
    }
    tick(done);
  }
}

// MTwrap's Enc_class: a worker. Plain data, never constructed (MTwrap maps
// its workers): Init() sets it up, do_process() allocates what it needs,
// and frees it as it ends, or Quit() wherever it was left.
template <int f_DEC>
struct Mp3Part : Coroutine {
  typedef mp3mod<f_DEC> Model;
  const PackArgs* args;
  Vec<uint8_t> in_;   // compressing, a part's records, all of them; decompressing, 64 KB of its stream
  Vec<uint8_t> out_;  // what the model made of a part
  Model* m;           // its model (pages of its own), from part to part
  uint64_t parts;     // the parts done (from another thread: done())
  PartProg prog;      // the input it stands for so far, the output, the frames (the progress line)
  uint32_t worker;    // its place among the workers (MTwrap's), dealt the parts in turn
  bool eof;           // its input has ended

  uint32_t Init(const PackArgs* a, uint32_t w) {
    coro_init();
    args = a;
    worker = w;
    in_.Init();
    out_.Init();
    m = nullptr;
    parts = 0;
    prog.set(0, 0, 0);
    eof = false;
    return 0;
  }
  void Quit() {
    in_.Quit();
    out_.Quit();
    page_free(m);
    m = nullptr;
  }

  void do_process() {
    m = (Model*)page_alloc(sizeof(Model));  // (zeroed, as mp3c's static one)
    m->Init();
    int r = run(IntTag<f_DEC>());
    Quit();
    for (;;) yield(this, r ? 3 : 0);
  }

  int fail(const char* why) {
    say_error(args->meter, "mp3pack: %s: %s\n", args->name, why);
    return 1;
  }
  // up to n bytes of the input, fewer only at its end (yield 1 for more)
  size_t read(uint8_t* p, size_t n) {
    size_t k = 0;
    while (k < n) {
      if (inpptr >= inpend) {
        if (eof) break;
        chkinp();
        if (inpptr >= inpend) {
          eof = true;
          break;
        }
      }
      size_t l = Min<size_t>(n - k, inpend - inpptr);
      memcpy(p + k, inpptr, l);
      inpptr += l, k += l;
    }
    return k;
  }
  // all n bytes into the output (yield 2 with each full buffer)
  void write(const uint8_t* p, size_t n) {
    while (n) {
      if (outptr >= outend) chkout();
      size_t l = Min<size_t>(n, outend - outptr);
      memcpy(outptr, p, l);
      outptr += l, p += l, n -= l;
    }
  }
  // the next n bytes of the input into in_ (false: there are fewer)
  bool take(uint64_t n) {
    if (n != (size_t)n) return false;
    in_.clear();
    while (in_.size() < n) {
      size_t k = in_.size(), l = (size_t)Min<uint64_t>(n - k, (uint64_t)1 << 20);
      in_.resize(k + l);
      size_t got = read(in_.data() + k, l);
      in_.resize(k + got);
      if (got < l) return false;
    }
    return true;
  }
  void rec(uint64_t csize, uint64_t dsize) {
    uint8_t h[kMtRec];
    put_le(h, csize, 8);
    put_le(h + 8, dsize, 8);
    put_le(h + 16, next_part(), 8);
    write(h, kMtRec);
  }
  // the place of the next part it has (worker w's k-th: w + kN)
  uint64_t next_part() const { return worker + parts * (uint64_t)args->threads; }
  // a part done, its record handed over now, its last buffer as far as it
  // is filled (a round's end, as d's worker ends one): so MTwrap puts each
  // record in its stream as its part is done
  void part_done() {
    __atomic_store_n(&parts, parts + 1, __ATOMIC_RELAXED);
    yield(this, 4);
  }
  uint64_t done() const { return __atomic_load_n(&parts, __ATOMIC_RELAXED); }

  // compressing: each part as the splitter frames it (u64 L, at, raw and
  // frames, then the records, L - 24 bytes) into its record, and as the
  // parts end, the record of the worker's end
  int run(IntTag<0>) {
    for (;;) {
      uint8_t h[kPartHead];
      size_t n = read(h, kPartHead);
      if (!n) {
        rec(0, 0);
        return 0;
      }
      uint64_t L = get_le(h, 8), raw = get_le(h + 16, 8), frames = get_le(h + 24, 8);
      if (n < (size_t)kPartHead || L < (uint64_t)kPartHead - 8 || !take(L - (kPartHead - 8)))
        return fail("internal: a part framed wrong");
      const uint64_t in0 = prog.in(), out0 = prog.out(), f0 = prog.frames();
      const size_t size = in_.size();
      // the part, a GB at a time (addinp() takes 32 bits), then its end: a
      // 0-length input with f_quit, where the model's records end
      // (read_frame_dump())
      size_t fed = 0;
      bool ended = false;
      auto feed = [&]() {
        if (fed < size) {
          size_t l = Min<size_t>(size - fed, (size_t)1 << 30);
          m->addinp(in_.data() + fed, (uint)l);
          fed += l;
          return true;
        }
        if (ended) return false;
        ended = true;
        m->f_quit = 1;
        m->addinp(in_.data() + size, 0);
        return true;
      };
      auto tick = [&](size_t done) {
        double f = size ? (double)(fed - (size_t)(m->inpend - m->inpptr)) / size : 1;
        prog.set(in0 + (uint64_t)(raw * f), out0 + kMtRec + done, f0 + (uint64_t)(frames * f));
        args->meter->poll();
      };
      const uint64_t kAny = ~(uint64_t)0 >> 1;  // (a part's compressed size: as it comes)
      if (!run_model(m, feed, out_, kAny, tick)) return fail("internal: a part's model stopped early");
      prog.set(in0 + raw, out0 + kMtRec + out_.size(), f0 + frames);
      rec(out_.size(), size);
      write(out_.data(), out_.size());
      part_done();
    }
  }

  // decompressing: each record into its part, at the end of which MTwrap
  // takes the next part from the next worker; to the record of the
  // worker's end, after which nothing
  int run(IntTag<1>) {
    for (;;) {
      uint8_t h[kMtRec];
      if (read(h, kMtRec) < (size_t)kMtRec) return fail("a broken or truncated stream: a worker's records cut short");
      uint64_t csize = get_le(h, 8), dsize = get_le(h + 8, 8);
      if (get_le(h + 16, 8) != next_part()) return fail("a broken stream: a part lost, added or moved");
      if (!csize) {  // the worker's end
        uint8_t x;
        if (dsize) return fail("a broken stream: a part's record");
        if (read(&x, 1)) return fail("a broken stream: data after a worker's end");
        return 0;
      }
      if (dsize > args->dump_max) return fail("a broken stream: a part's record");
      // the part compressed, read as its model asks for it, none of it held
      // but 64 KB (whatever size the record says), then its padding
      uint64_t left = csize;
      in_.resize((size_t)1 << 16);
      int pad = 0;
      auto feed = [&]() {
        if (left) {
          size_t got = read(in_.data(), (size_t)Min<uint64_t>(left, in_.size()));
          left -= got;
          if (got) {
            m->addinp(in_.data(), (uint)got);
            return true;
          }
          left = 1;  // (cut short: what was not there, after the padding)
        }
        if (pad++) return false;
        m->addinp((byte*)kPad, sizeof kPad);
        return true;
      };
      const uint64_t in0 = prog.in(), out0 = prog.out(), f0 = prog.frames();
      auto tick = [&](size_t done) {
        prog.set(in0 + kMtRec + (csize - left), out0 + done, f0);
        args->meter->poll();
      };
      if (!run_model(m, feed, out_, dsize, tick) || left || out_.size() != dsize)
        return fail("a broken stream: a part does not decompress as it was compressed");
      prog.set(in0 + kMtRec + csize, out0 + dsize, f0);
      write(out_.data(), out_.size());
      part_done();
    }
  }
};

// c: the splitter into MTwrap's coder; d: MTwrap's decoder into the joiner
typedef CoroutinePair<Mp3Split, MTwrap<0, Mp3Part<0>, const PackArgs*>> MtEnc;
typedef CoroutinePair<MTwrap<1, Mp3Part<1>, const PackArgs*>, Mp3Join> MtDec;

// What the workers did: the input they stand for so far and their output
// (the progress line; from any thread), their frames and parts, and
// whether one failed (and said why)
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
// whether the workers had the parts as they are dealt, in turn: of P in
// all, worker i the i-th of each round of N, one more where P % N is past
// it. (Each worker's records say their places, to its end's.)
template <class M>
bool mt_dealt(const M& d) {
  uint64_t P = mt_work(d).parts, N = d.th_N;
  for (uint i = 0; i < d.th_N; i++)
    if (d.C1[i].done() != P / N + (i < P % N)) return false;
  return true;
}

}  // namespace mp3pack
