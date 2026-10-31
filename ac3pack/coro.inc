// coro.inc -- the compressor and the decompressor, as coroutines (Lib3's, as m4vpack's): a run,
// on the buffers of its input and output, at a level.

// The stream: "ac3pack", the format's version, the model's level; then the arithmetic coded data:
// the chunks (kind, then the records of a frame, or a length and the bytes of bytes kept as
// bytes), the end, and the CRC-32 of the stream (32 bits at 1/2 each).
enum { AC3P_MAGIC = 7, AC3P_AT_VERSION = AC3P_MAGIC, AC3P_AT_LEVEL, AC3P_HEAD };  // the header's bytes
static const U8 magic[AC3P_MAGIC] = {'a', 'c', '3', 'p', 'a', 'c', 'k'};
constexpr int AC3P_VERSION = 1;  // the format's

// a compression's statistics (its summary line)
struct Stats {
  U64 frames, ac3, eac3, dep, packed, raw_parse, raw_verify;
  double in_bytes, raw_bytes, frame_bytes;
  U64 parts;    // -t: the parts
  int threads;  // and the threads

  void add(const Stats& s) {  // a part's
    frames += s.frames;
    ac3 += s.ac3;
    eac3 += s.eac3;
    dep += s.dep;
    packed += s.packed;
    raw_parse += s.raw_parse;
    raw_verify += s.raw_verify;
    in_bytes += s.in_bytes;
    raw_bytes += s.raw_bytes;
    frame_bytes += s.frame_bytes;
  }

  // c's summary line (out_bytes: the compressed size, at the level), and with -s the bytes by kind
  // of syntax element, the coder's cost and orig (CAT_*)
  void report(const char* name, rp<const double> cost, rp<const double> orig, double out_bytes, int level, int stats) const {
    static const char* const cat_name[NCAT] = {"bytes kept as bytes", "header, CRCs, trailer", "side information",
                                               "exponents", "mantissas", "mantissas' signs", "AHT", "skip fields",
                                               "bits after the last block"};
    int i;
    fprintf(stderr, "%s: %.0f -> %.0f bytes (%.2f%%, -%d); %llu frames (AC-3 %llu, E-AC-3 %llu, %llu of them dependent), %llu packed",
            name, in_bytes, out_bytes, in_bytes ? PERCENT * out_bytes / in_bytes : 0.0, level, (unsigned long long)frames,
            (unsigned long long)ac3, (unsigned long long)eac3, (unsigned long long)dep, (unsigned long long)packed);
    if (frames > packed)
      fprintf(stderr, ", %llu kept as bytes (%llu unparsable, %llu not reproduced)", (unsigned long long)(frames - packed),
              (unsigned long long)raw_parse, (unsigned long long)raw_verify);
    fprintf(stderr, "; %.0f bytes between frames", raw_bytes);
    if (threads) fprintf(stderr, "; %llu parts on %d threads", (unsigned long long)parts, threads);
    fprintf(stderr, "\n");
    if (stats) {
      double o = 0, p = 0;
      fprintf(stderr, "  %-28s %10s %10s\n", "bytes", "input", "packed");
      for (i = 0; i < NCAT; i++) {
        if (i == CAT_SIGN) continue;  // (with the mantissas, whose fields have them)
        const double c = cost[i] + (i == CAT_MANT ? cost[CAT_SIGN] : 0);
        fprintf(stderr, "  %-28s %10.0f %10.0f %7.2f%%\n", cat_name[i], orig[i] / BYTE_BITS, c / BYTE_BITS,
                orig[i] ? PERCENT * c / orig[i] : 0.0);
        if (i == CAT_MANT) fprintf(stderr, "    %-26s %10s %10.0f\n", "of them the signs", "", cost[CAT_SIGN] / BYTE_BITS);
        o += orig[i];
        p += c;
      }
      fprintf(stderr, "  %-28s %10.0f %10.0f %7.2f%%\n", "total", o / BYTE_BITS, p / BYTE_BITS, o ? PERCENT * p / o : 0.0);
    }
  }
};

#include "ac3coro.inc"  // Ac3Coro: a run of c or d

// A run's coroutine, on pages of its own: zeroed, and taking memory only where the run touches
// them. coro_new() makes it (plain data: then Init()), coro_free() frees it after its Quit().
template <class T>
T* coro_new(void) {
  return (T*)map_pages(sizeof(T));
}
template <class T>
void coro_free(T* p) {
  unmap_pages(p, sizeof(T));
}

// The output of a call: a file, or (c -v) the input to compare with
struct Sink {
  FILE* f;
  int compare;
  U64 pos, diff_at;
  int differs, err;

  void write(pcU8 p, size_t n) {
    static U8 cmp[IO_BUF];  // (the main thread's)
    if (!compare) {
      if (n && fwrite(p, 1, n, f) != n) err = 1;
      pos += n;
      return;
    }
    while (n && !differs) {
      const size_t k = n < sizeof(cmp) ? n : sizeof(cmp);
      const size_t got = fread(cmp, 1, k, f);
      size_t i;
      for (i = 0; i < got && cmp[i] == p[i]; i++) {}
      if (i < k) {
        differs = 1;
        diff_at = pos + i;
        return;
      }
      pos += k;
      p += k;
      n -= k;
    }
  }
};

// Runs the coroutine c in the buffer protocol to its end: in() gives it its next input when it
// asks (Y_IN), out(more) takes what it put in its output buffer when that is full (Y_OUT, more:
// and gives it the next) or as it ends (Y_END, or Y_FAIL after an error: what drive() returns);
// show(code) after each yield (what it tells, Y_AUDIO; the progress line)
template <class C, class In, class Out, class Show>
uint drive(C& c, In in, Out out, Show show) {
  for (;;) {
    const uint r = c.coro_call(&c);
    if (r == Y_IN) in();
    else if (r != Y_AUDIO) out(r == Y_OUT);
    show(r);
    if (r != Y_IN && r != Y_OUT && r != Y_AUDIO) return r;
  }
}

// c on files: its input from f (the n bytes at pre read already, first), its output to s, and
// show(code) after each yield
static U8 io_bufs[2][IO_BUF];  // (the main thread's: input, output)
template <class C, class Show>
uint run_file(C& c, FILE* f, Sink& s, Show show, pcU8 pre = 0, size_t n = 0) {
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
        s.write(c.outbeg, c.getoutsize());
        if (more) c.addout(ob, kIo);
      },
      show);
}
