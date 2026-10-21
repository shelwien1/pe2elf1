// h264p on threads (c and x with -t N, and d of what that wrote): the stream
// cut into parts by h264split.inc's splitter (H264Split, on the main
// thread) -- a part ends at the first IDR or I access unit at least -c's
// part size after its start, or at the first picture at least mt_cap()
// after it, and the parameter sets a part needs are copied in at its
// start, so that each is a stream by itself -- and the parts dealt in
// turn to N workers by MTwrap (MTwrap_v3, its parts mode: MTwrap.md section
// 11). A worker codes its parts one after the other, each by a model of its
// own: H264pCoro (coro.inc), run as a coroutine of the worker's on the part
// read whole into memory. A worker's stream is a record for each part
// (rzwrap_h264's):
//
//   u32 g_size  the record's size, these 16 bytes included
//   u32 nins    the parameter sets the splitter inserted, removed after decoding
//   u64 plen    the size of the part as compressed (with those sets)
//   g_size-16 bytes: the part compressed, a stream of h264p's (magic, version, level, ...)
//
// (little-endian), and MTwrap's container holds the N streams; h264p's
// header goes before it (kMtMagic, mt_head()). Decoding needs the same N
// (the header has it); each part ends a round of MTwrap's decoder (yield 4),
// which takes the parts from the workers in turn as they were dealt.

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
// unit to cut at), for the memory a part takes and its record's 32 bits
uint64_t mt_cap(uint32_t part) { return std::min<uint64_t>(4 * (uint64_t)part, (uint64_t)1 << 31); }

// A worker's counts for the progress line: its input and its output so
// far (their parts' and the part's under way)
struct PartProg {
  std::atomic<uint64_t> in, out;
  void set(uint64_t i, uint64_t o) {
    in.store(i, std::memory_order_relaxed);
    out.store(o, std::memory_order_relaxed);
  }
};

// A run of a codec, H264pCoro<f_DEC, LV>, on a part in memory: all of in
// its input, its output into out (grown as it fills, handed to it 64 KB at
// a time); the run's status (0, 1 after an error, kRestart). As it goes,
// its input and output from in0 and out0 on are the worker's counts, and a
// line of progress may be due.
template <class C>
int run_part(const H264pArgs& a, std::vector<uint8_t>& in, std::vector<uint8_t>& out, PartProg& prog, uint64_t in0,
             uint64_t out0) {
  CoroObj<C> c;
  c->Init(&a);
  size_t pos = 0;
  auto feed = [&]() {  // the next piece of the part (addinp() takes 32 bits), or its end
    size_t l = std::min(in.size() - pos, (size_t)1 << 30);
    if (!l) c->f_quit = 1;
    c->addinp(in.data() + pos, (uint)l);
    pos += l;
  };
  out.resize(1 << 16);
  c->addout(out.data(), (uint)out.size());
  feed();
  for (;;) {
    uint r = c->coro_call(c.get());
    prog.set(in0 + c->n_in, out0 + c->n_out);
    progress_poll(a.meter);
    if (r == 1) {
      feed();
      continue;
    }
    size_t done = (size_t)(c->outptr - out.data());
    if (r != 2) {  // the end, or an error
      out.resize(done);
      break;
    }
    if (done == out.size()) out.resize(done * 2);  // (the output full: twice the room)
    c->addout(out.data() + done, (uint)std::min(out.size() - done, (size_t)1 << 16));
  }
  return c->status;
}

// MTwrap's Enc_class: a worker. Plain data, never constructed (MTwrap maps
// its workers): Init() sets it up, do_process() allocates what it needs,
// and frees it as it ends, or Quit() wherever it was left.
template <int f_DEC>
struct H264pPart : CoroIo {
  H264pArgs args;
  std::vector<uint8_t>* in_;   // a part, or its record: all of it, read before its model starts
  std::vector<uint8_t>* out_;  // what the part's model made of it
  uint64_t parts;              // the parts done
  PartProg prog;               // the input coded so far and the output (the progress line)

  uint32_t Init(const H264pArgs* a) {
    coro_init();
    args = *a;
    in_ = out_ = nullptr;
    parts = 0;
    prog.set(0, 0);
    return 0;
  }
  void Quit() {
    delete in_;
    delete out_;
    in_ = out_ = nullptr;
  }

  void do_process() {
    int r = 1;
    io_init();
    try {
      in_ = new std::vector<uint8_t>;
      out_ = new std::vector<uint8_t>;
      r = run(std::integral_constant<int, f_DEC>());
    } catch (const std::exception& e) {  // no memory (nothing in a handler may yield)
      say_error(args.meter, "h264p: %s: %s\n", args.name, e.what());
    }
    Quit();
    for (;;) yield(static_cast<Coroutine*>(this), r ? 3 : 0);
  }

  int fail(const char* why) {
    say_error(args.meter, "h264p: %s: %s\n", args.name, why);
    return 1;
  }
  // the next n bytes of the input into in_ (false: there are fewer)
  bool take(uint64_t n) {
    if (n != (size_t)n) return false;
    in_->resize((size_t)n);
    return read(in_->data(), (size_t)n) == n;
  }
  // the part's model run on in_ into out_, at the level of a.level (the
  // counts from in0 and out0 on)
  template <class LV>
  struct Lv {
    static int run(H264pPart& w, const H264pArgs& a, uint64_t in0, uint64_t out0) {
      return run_part<H264pCoro<f_DEC, LV>>(a, *w.in_, *w.out_, w.prog, in0, out0);
    }
  };
  int code(const H264pArgs& a, uint64_t in0, uint64_t out0) {
    int r = with_level<Lv>(a.level, *this, a, in0, out0);
    if (r >= 0) return r;
    say_error(args.meter, "h264p: %s: a part compressed at level -%d, which this build has not\n", args.name, a.level);
    return 1;
  }

  // compressing: each part as the splitter frames it (u64 L, u32 nins,
  // then the part, L-4 bytes) into its record
  int run(std::integral_constant<int, 0>) {
    for (;;) {
      uint8_t h[16];
      size_t n = read(h, 12);
      if (!n) return 0;  // the end
      if (n < 12) return fail("internal: a part framed wrong");
      uint64_t L;
      uint32_t nins;
      memcpy(&L, h, 8);
      memcpy(&nins, h + 8, 4);
      uint64_t plen = L - 4;
      if (L < 4 || !take(plen)) return fail("internal: a part framed wrong");
      // (the part can be read again: the model pass checks the slices it
      // can, and if one does not come out as it is, the dry runs write back
      // each, again)
      H264pArgs a = args;
      a.check = true;
      uint64_t in0 = prog.in.load(std::memory_order_relaxed), out0 = prog.out.load(std::memory_order_relaxed);
      int r = code(a, in0, out0);
      if (r == kRestart) {
        a.check = false;
        r = code(a, in0, out0);
      }
      if (r) return 1;
      prog.set(in0 + plen, out0 + 16 + out_->size());
      if (out_->size() > 0xFFFFFFFFu - 16) return fail("a part compressed to 4 GB or more");
      put_le(h, out_->size() + 16, 4);
      put_le(h + 4, nins, 4);
      put_le(h + 8, plen, 8);
      write(h, 16);
      write(out_->data(), out_->size());
      parts++;
    }
  }

  // decompressing: each record into its part, the inserted parameter sets
  // removed, at the end of which MTwrap takes the next part from the next
  // worker
  int run(std::integral_constant<int, 1>) {
    for (;;) {
      uint8_t h[16];
      size_t n = read(h, 16);
      if (!n) return 0;  // the end
      if (n < 16) return fail("a part's record cut short");
      uint64_t g_size = get_le(h, 4), nins = get_le(h + 4, 4), plen = get_le(h + 8, 8);
      if (g_size < 16 || !take(g_size - 16)) return fail("a part's record cut short or broken");
      H264pArgs a = args;
      a.level = cm_level(in_->data(), std::min(in_->size(), sizeof kCmMagic + 1), args.name, args.meter);
      uint64_t in0 = prog.in.load(std::memory_order_relaxed), out0 = prog.out.load(std::memory_order_relaxed);
      if (a.level < 0 || code(a, in0 + 16, out0)) return 1;
      size_t ins, pos;
      if (out_->size() != plen || !inserted(nins, ins, pos)) return fail("a part did not decompress as it was compressed");
      prog.set(in0 + g_size, out0 + out_->size() - (pos - ins));
      write(out_->data(), ins);
      write(out_->data() + pos, out_->size() - pos);
      parts++;
      before_yield();
      yield(static_cast<Coroutine*>(this), 4);
    }
  }

  // Where the nins parameter sets the splitter inserted are in the part
  // decoded (out_): [ins, pos), after the access unit delimiter if the part
  // starts with one, and each a 4-byte start code and an SPS, a PPS, an SPS
  // extension or a subset SPS, up to its last nonzero byte (h264split r's
  // checks); false if they are not there
  bool inserted(uint64_t nins, size_t& ins, size_t& pos) const {
    static const uint8_t sc4[4] = {0, 0, 0, 1};
    const uint8_t* b = out_->data();
    size_t n = out_->size(), j;
    ins = pos = 0;
    if (!nins) return true;
    size_t sc = start_code(b, n, 0);
    if (sc + 4 > n) return false;
    if ((b[sc + 3] & 31) == 9) {
      if ((j = start_code(b, n, sc + 4)) >= n) return false;
      while (j > sc + 4 && !b[j - 1]) j--;
      ins = j;
    }
    pos = ins;
    for (uint64_t k = 0; k < nins; k++) {
      if (pos + 5 > n || memcmp(b + pos, sc4, 4)) return false;
      int t = b[pos + 4] & 31;
      if (!(t == 7 || t == 8 || t == 13 || t == 15)) return false;
      if ((j = start_code(b, n, pos + 5)) >= n) return false;
      while (j > pos + 5 && !b[j - 1]) j--;
      pos = j;
    }
    return true;
  }
  // the first 00 00 01 at i or after (n if none)
  static size_t start_code(const uint8_t* b, size_t n, size_t i) {
    for (; i + 2 < n; i++)
      if (!b[i] && !b[i + 1] && b[i + 2] == 1) return i;
    return n;
  }
};

// c, x: the splitter into MTwrap's coder; d, x: MTwrap's decoder
typedef CoroutinePair<H264Split, MTwrap<0, H264pPart<0>, const H264pArgs*>> MtEnc;
typedef MTwrap<1, H264pPart<1>, const H264pArgs*> MtDec;

// the input the workers have coded so far and their output (the progress
// line; from any thread)
template <class M>
void mt_done(const M& m, uint64_t& in, uint64_t& out) {
  in = out = 0;
  for (uint i = 0; i < m.th_N; i++) {
    in += m.C1[i].prog.in.load(std::memory_order_relaxed);
    out += m.C1[i].prog.out.load(std::memory_order_relaxed);
  }
}
// whether a worker failed (and said why)
template <class M>
bool mt_failed(const M& m) {
  for (uint i = 0; i < m.th_N; i++)
    if (m.C1[i].f_fail) return true;
  return false;
}
// the parts the workers did
template <class M>
uint64_t mt_parts(const M& m) {
  uint64_t n = 0;
  for (uint i = 0; i < m.th_N; i++) n += m.C1[i].parts;
  return n;
}
// An MTwrap stage that the pair it was in stopped driving (the other stage
// failed) run to its end, its input ended and its output dropped, so that
// its workers end and Quit() can join them (nothing to do where they have)
template <class M>
void mt_drain(M& m) {
  bool busy = false;
  for (uint i = 0; i < m.th_N; i++) busy = busy || !m.C1[i].f_done;
  if (!busy) return;
  std::vector<byte> sink((size_t)1 << 16);
  m.f_quit = 1;
  m.addinp(sink.data(), 0);
  m.addout(sink.data(), (uint)sink.size());
  for (;;) {
    uint r = m.coro_call(&m);
    if (r == 1)
      m.addinp(sink.data(), 0);
    else if (r == 2)
      m.addout(sink.data(), (uint)sink.size());
    else
      break;
  }
}
