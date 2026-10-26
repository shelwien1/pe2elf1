// The calls c and d (aacpack's drivers.inc): their coroutines run on the
// files, with a progress line, with one model or on threads (mt.inc).
//
// The compressed stream:
//
//   "mp3pk", the version, the threads (1: one model) and the part size in
//   KB (u32; 0 with one model)
//   the stream of one model (mp3c's), or MTwrap's container of the
//   workers' streams (mt.inc)
//   the CRC-32 of the file (u32)
//
// d takes the last 4 bytes of its input for the CRC, and the rest for the
// stream; a model whose stream ends before its input does, or that wants
// more than 8 bytes past it, finds it broken. The outside data is the
// second file: c writes it as mp3rw's .out, d reads it.

namespace mp3pack {

const char kMagic[] = "mp3pk";
const int kVersion = 1;
const int kHead = (int)sizeof kMagic - 1 + 1 + 1 + 4;  // the magic, the version, the threads, the part size
const uint kIo = 1 << 16;  // the size of a call's input buffer and of its output buffer

// c and d of one model: the splitter into the model, the model into the
// joiner (Lib3's CoroutinePair)
typedef CoroutinePair<Mp3Split, mp3mod<0>> Enc1;
typedef CoroutinePair<mp3mod<1>, Mp3Join> Dec1;

// A chain of coroutines on pages of its own, zeroed (plain data: Init()s
// set it up), freed after
template <class T>
T* coro_new() {
  return (T*)page_alloc(sizeof(T));
}
template <class T>
void coro_free(T* p) {
  page_free(p);
}

// What a call has done (the progress line, read from any thread): its
// input's bytes read, its output's written and the frames (of the
// splitter or the joiner), as the main thread sets them; and with
// threads, c's workers' (what they have coded)
struct Counts {
  uint64_t in_, out_, frames_;
  bool c;
  MtWork (*work)(const void*);
  const void* mt;

  void Init(bool _c) {
    in_ = out_ = frames_ = 0;
    c = _c;
    work = nullptr;
    mt = nullptr;
  }
  void set(uint64_t i, uint64_t o, uint64_t f) {
    __atomic_store_n(&in_, i, __ATOMIC_RELAXED);
    __atomic_store_n(&out_, o, __ATOMIC_RELAXED);
    __atomic_store_n(&frames_, f, __ATOMIC_RELAXED);
  }
  uint64_t in() const { return __atomic_load_n(&in_, __ATOMIC_RELAXED); }
  uint64_t out() const { return __atomic_load_n(&out_, __ATOMIC_RELAXED); }
  uint64_t frames() const { return __atomic_load_n(&frames_, __ATOMIC_RELAXED); }
};
void call_counts(void* p, uint64_t& raw, uint64_t& packed, uint64_t& pos, uint64_t& frames) {
  const Counts& k = *(const Counts*)p;
  if (k.work) {  // (c on threads: what the workers have coded)
    MtWork w = k.work(k.mt);
    raw = pos = w.in;
    packed = kHead + w.out;
    frames = w.frames;
    return;
  }
  raw = k.c ? k.in() : k.out();
  packed = k.c ? k.out() : k.in();
  pos = k.in();
  frames = k.frames();
}
template <class M>
MtWork work_of(const void* m) {
  return mt_work(*(const M*)m);
}

// A call's progress line, tagged tag, through the input f: shown with -p,
// or where stderr is a terminal
void progress_start(Progress& pr, const Options& opt, const char* tag, FILE* f) {
  pr.Init();
#ifdef _WIN32
  bool tty = _isatty(_fileno(stderr)) != 0;
#else
  bool tty = isatty(fileno(stderr)) != 0;
#endif
  if (opt.progress || tty) pr.start(tag, file_size(f));
}

// what MTwrap's Init() returned, said (false: nothing to say)
bool mt_init_failed(uint r, int threads, Progress& pr) {
  if (r == 7)
    say_error(&pr, "mp3pack: cannot start %d threads\n", threads);
  else if (r)
    say_error(&pr, "mp3pack: no memory for the buffers of %d threads\n", threads);
  return r != 0;
}

// c: the chain e (Enc1 or MtEnc, set up) on the input f, its output (the
// stream) into o after the header, then the CRC of the input; the counts
// in k as it goes; frames, the splitter's. The status (0, or 1 said; a
// write error is said as o closes).
template <class E>
int code_run(E& e, FILE* f, const char* in_name, Output& o, Progress& pr, Counts& k, const uint64_t& frames,
             const uint8_t* head) {
  uint8_t* ib = (uint8_t*)mem_alloc(2 * kIo);
  uint8_t* ob = ib + kIo;
  Crc32 crc;
  crc.Init();
  uint64_t in = 0, out = kHead;
  o.put(head, kHead);
  e.coro_init();
  e.addout(ob, kIo);
  uint r;
  for (;;) {
    r = e.coro_call(&e);
    if (r == 1) {
      uint l = (uint)fread(ib, 1, kIo, f);
      crc.add(ib, l);
      in += l;
      if (!l) e.f_quit = 1;
      e.addinp(ib, l);
    } else if (r == 2 || r == 0) {
      uint l = e.getoutsize();
      o.put(e.outbeg, l);
      out += l;
      if (r == 0) break;
      e.addout(ob, kIo);
    } else {
      break;
    }
    k.set(in, out, frames);
    pr.poll();
  }
  uint8_t t[4];
  put_le(t, crc.value(), 4);
  o.put(t, 4);
  k.set(in, out + 4, frames);
  mem_free(ib);
  if (r != 0) {
    if (!pr.stopped()) say_error(&pr, "mp3pack: %s: internal: the compressor stopped early\n", in_name);
    return 1;
  }
  return 0;
}

// c
int code(const char* in_name, const char* out_name, const char* x_name, const Options& opt) {
  if (!strcmp(out_name, x_name) || (strcmp(out_name, "-") && strcmp(x_name, "-") && same_names(out_name, x_name))) {
    fprintf(stderr, "mp3pack: the output and the outside data are the same file\n");
    return 2;
  }
  FILE* f = open_input(in_name);
  if (!f) return 1;
  Output o, x;
  o.Init();
  x.Init();
  if (!o.open(out_name, &f, 1, &in_name) || !x.open(x_name, &f, 1, &in_name)) {
    o.Quit();
    if (f != stdin) fclose(f);
    return 1;
  }
  Progress pr;
  progress_start(pr, opt, "enc", f);
  FileOut xo;
  xo.Init(x.g);
  Counts k;
  k.Init(true);
  uint8_t head[kHead];
  memcpy(head, kMagic, sizeof kMagic - 1);
  head[sizeof kMagic - 1] = (uint8_t)kVersion;
  head[sizeof kMagic] = (uint8_t)opt.threads;
  put_le(head + sizeof kMagic + 1, opt.threads > 1 ? opt.part >> 10 : 0, 4);
  int r;
  uint64_t frames = 0, runs = 0, parts = 0;
  if (opt.threads <= 1) {
    Enc1* e = coro_new<Enc1>();
    e->M1.Init(&xo, false, 0);
    e->M2.Init();
    pr.counts(call_counts, &k);
    r = code_run(*e, f, in_name, o, pr, k, e->M1.frames, head);
    pr.counts(nullptr, nullptr);
    frames = e->M1.frames, runs = e->M1.runs;
    e->M1.Quit();
    coro_free(e);
  } else {
    typedef MTwrap<0, Mp3Part<0>, const PackArgs*> W;
    PackArgs a = {&pr, in_name, opt.threads, 0};
    MtEnc* e = coro_new<MtEnc>();
    e->M1.Init(&xo, true, opt.part);
    if (mt_init_failed(e->M2.Init(opt.threads, kMtRing, &a, 1), opt.threads, pr)) {
      r = 1;  // (threads already started wait on input: they end with the process)
    } else {
      k.work = work_of<W>;
      k.mt = &e->M2;
      pr.counts(call_counts, &k);
      r = code_run(*e, f, in_name, o, pr, k, e->M1.frames, head);
      if (mt_work(e->M2).failed) r = 1;
      pr.counts(nullptr, nullptr);
      frames = e->M1.frames, runs = e->M1.runs, parts = e->M1.nparts;
      e->M2.Quit();  // (the threads joined)
    }
    e->M1.Quit();
    coro_free(e);
  }
  xo.flush();
  if (xo.err && !r) {
    say_error(&pr, "mp3pack: %s: write error\n", x_name);
    r = 1;
  }
  uint64_t outside = xo.total;
  xo.Quit();
  if (!close_input(f, in_name, &pr)) r = 1;
  // (both outputs committed, or neither: their write errors first)
  Output* os[2] = {&o, &x};
  for (Output* p : os)
    if (!r && (fflush(p->g) || ferror(p->g))) {
      say_error(&pr, "mp3pack: %s: write error\n", p->name);
      r = 1;
    }
  uint64_t in = k.in(), out = k.out();
  int ro = o.close(r, false, &pr);
  r = x.close(r | ro, false, &pr) | ro;
  pr.end(r, in, out, frames);
  pr.Quit();
  if (!r && opt.verbose) {
    fprintf(stderr, "%" PRIu64 " -> %" PRIu64 " bytes (%.2f%%), and %" PRIu64 " bytes of outside data; %" PRIu64
                    " frames in %" PRIu64 " runs",
            in, out, in ? 100.0 * out / in : 0.0, outside, frames, runs);
    if (parts) fprintf(stderr, "; %" PRIu64 " parts on %d threads", parts, opt.threads);
    fprintf(stderr, "\n");
  }
  return r;
}

// d's input: the stream but its last 4 bytes, which are held back (the
// CRC), a piece at a time
struct HeldInput {
  FILE* f;
  uint8_t* b;   // kIo + 4 bytes: what was read and not given, then what is read
  size_t held;  // the bytes not given (at most 4: the last read), at b + from
  size_t from;
  bool eof;     // the input has ended: the held are all that is left
  uint64_t in;  // the bytes read

  void Init(FILE* g) {
    f = g;
    b = (uint8_t*)mem_alloc(kIo + 4);
    held = from = 0;
    eof = false;
    in = 0;
  }
  void Quit() { mem_free(b); }
  // the next piece of the stream at p (0: its end). (A piece is taken
  // whole before the next is asked for.)
  size_t next(uint8_t*& p) {
    for (;;) {
      if (from) memmove(b, b + from, held), from = 0;
      if (eof) return 0;
      size_t l = fread(b + held, 1, kIo, f);
      in += l;
      if (!l) {
        eof = true;
        return 0;
      }
      size_t n = held + l, give = n > 4 ? n - 4 : 0;
      held = n - give, from = give;
      if (give) {
        p = b;
        return give;
      }
    }
  }
  // the CRC, at the end (false: there are not 4 bytes left for it)
  bool crc(uint32_t& v) {
    uint8_t* p;
    if (next(p) || held != 4) return false;
    v = (uint32_t)get_le(b, 4);
    return true;
  }
};

// d: the chain d (Dec1 or MtDec, set up) on the stream hi gives, its
// output (the file) into o; the counts in k as it goes; frames, the
// joiner's; one: a model (else MTwrap's decoder) takes the stream first.
// The status (0, or 1 said); out, the bytes written.
template <class D>
int decode_run(D& d, HeldInput& hi, const char* in_name, Output& o, Progress& pr, Counts& k, const uint64_t& frames,
               bool one, const char* const& jwhy, uint64_t& out) {
  const char* why = nullptr;
  uint8_t* ob = (uint8_t*)mem_alloc(kIo);
  Crc32 crc;
  crc.Init();
  out = 0;
  int stage = 0;  // the stream given; 1: then the padding (one); 2: then its end (threads)
  d.coro_init();
  d.addout(ob, kIo);
  uint r;
  for (;;) {
    r = d.coro_call(&d);
    if (r == 1) {
      uint8_t* p;
      size_t l = stage ? 0 : hi.next(p);
      if (l) {
        d.addinp(p, (uint)l);
      } else if (stage == 0 && one) {
        stage = 1;
        d.addinp((byte*)kPad, sizeof kPad);
      } else if (stage == 0) {
        stage = 2;
        d.f_quit = 1;
        d.addinp(ob, 0);
      } else {
        why = "a broken or truncated stream";
        r = 3;
        break;
      }
    } else if (r == 2 || r == 0) {
      uint l = d.getoutsize();
      crc.add(d.outbeg, l);
      o.put(d.outbeg, l);
      out += l;
      if (r == 0) break;
      d.addout(ob, kIo);
    } else {
      break;
    }
    k.set(hi.in, out, frames);
    pr.poll();
  }
  k.set(hi.in, out, frames);
  mem_free(ob);
  if (r == 0) {
    // the stream taken to its end, and no more: then the CRC
    uint8_t* p;
    uint32_t v;
    if (stage == 0 && (d.inpptr < d.inpend || hi.next(p)))
      why = "data after the end of the stream";
    else if (!hi.crc(v))
      why = "a truncated stream";
    else if (v != crc.value())
      why = "the file restored is not the one compressed (a wrong outside data file, or a broken stream)";
    else
      return 0;
  }
  if (jwhy) why = jwhy;  // (the joiner's: the outside data, or a frame)
  if (!why) why = "a broken or truncated stream";
  if (!pr.stopped()) say_error(&pr, "mp3pack: %s: %s\n", in_name, why);
  return 1;
}

// The end of a call whose threads cannot be joined (d's joiner failed while
// MTwrap went on: its workers wait on rings no one serves): its output
// gone, the process ends, and the threads with it (they hold the call's
// progress line, and memory, that a return would free)
[[noreturn]] void abandon(Output& o, Progress& pr) {
  o.close(1, false, &pr);
  pr.end_line();
  fflush(stderr);
  _exit(1);
}

// d
int decode(const char* in_name, const char* x_name, const char* out_name, const Options& opt) {
  if (!strcmp(in_name, "-") && !strcmp(x_name, "-")) {
    fprintf(stderr, "mp3pack: the input and the outside data cannot both be stdin\n");
    return 2;
  }
  FILE* f = open_input(in_name);
  if (!f) return 1;
  FILE* xf = open_input(x_name);
  if (!xf) {
    if (f != stdin) fclose(f);
    return 1;
  }
  Output o;
  o.Init();
  FILE* ins[2] = {f, xf};
  const char* in_names[2] = {in_name, x_name};
  if (!o.open(out_name, ins, 2, in_names)) {
    if (f != stdin) fclose(f);
    if (xf != stdin) fclose(xf);
    return 1;
  }
  uint8_t head[kHead];
  size_t got = fread(head, 1, kHead, f);
  int threads = got == (size_t)kHead ? head[sizeof kMagic] : 0;
  uint64_t part = (uint64_t)get_le(head + sizeof kMagic + 1, 4) << 10;
  const char* bad = nullptr;
  if (got < sizeof kMagic - 1 || memcmp(head, kMagic, sizeof kMagic - 1))
    bad = "not a stream of mp3pack";
  else if (got < (size_t)kHead)
    bad = "a truncated stream";
  else if (head[sizeof kMagic - 1] != kVersion)
    bad = "a stream of another version of mp3pack";
  else if (threads < 1 || threads > 64 || (threads > 1 && !part))
    bad = "a broken stream (its header)";
  if (bad) {
    fprintf(stderr, "mp3pack: %s: %s\n", in_name, bad);
    if (xf != stdin) fclose(xf);
    if (f != stdin) fclose(f);
    return o.close(1, false, nullptr);
  }
  Progress pr;
  progress_start(pr, opt, "dec", f);
  HeldInput hi;
  hi.Init(f);
  FileIn xi;
  xi.Init(xf);
  Counts k;
  k.Init(false);
  pr.counts(call_counts, &k);
  uint64_t out = 0, frames = 0, parts = 0;
  int r;
  if (threads == 1) {
    Dec1* d = coro_new<Dec1>();
    d->M1.Init();
    d->M2.Init(&xi);
    r = decode_run(*d, hi, in_name, o, pr, k, d->M2.frames, true, d->M2.why, out);
    frames = d->M2.frames;
    coro_free(d);
  } else {
    PackArgs a = {&pr, in_name, threads, dump_max(part)};
    MtDec* d = coro_new<MtDec>();
    d->M2.Init(&xi);
    if (mt_init_failed(d->M1.Init(threads, kMtRing, &a, 1), threads, pr)) {
      abandon(o, pr);  // (threads already started wait on input)
    } else {
      r = decode_run(*d, hi, in_name, o, pr, k, d->M2.frames, false, d->M2.why, out);
      if (r && d->M1.state != 0 && d->M1.state != 3) abandon(o, pr);  // (MTwrap's run not at its end)
      if (!r && !mt_dealt(d->M1)) {
        say_error(&pr, "mp3pack: %s: a broken stream: parts lost\n", in_name);
        r = 1;
      }
      frames = d->M2.frames;
      parts = mt_work(d->M1).parts;
      d->M1.Quit();
    }
    coro_free(d);
  }
  pr.counts(nullptr, nullptr);
  uint64_t in = hi.in + kHead, outside = xi.total;
  hi.Quit();
  xi.Quit();
  if (!close_input(xf, x_name, &pr)) r = 1;
  if (!close_input(f, in_name, &pr)) r = 1;
  r = o.close(r, false, &pr);
  pr.end(r, out, in, frames);
  pr.Quit();
  if (!r && opt.verbose) {
    fprintf(stderr, "%" PRIu64 " bytes, of %" PRIu64 " and %" PRIu64 " of outside data; %" PRIu64 " frames", out, in,
            outside, frames);
    if (parts) fprintf(stderr, "; %" PRIu64 " parts on %d threads", parts, threads);
    fprintf(stderr, "\n");
  }
  return r;
}

}  // namespace mp3pack
