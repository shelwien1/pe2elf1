// coro.inc -- the compressor and the decompressor, as coroutines (Lib3's, as h264p's): a run,
// on the buffers of its input and output, at a level.

// The stream: "m4vpack", the format's version, the model's level; then the arithmetic coded data:
// the chunks (kind, then the records of a VOP or a raw chunk's length and bytes), the end, and
// the CRC-32 of the stream (32 bits at 1/2 each).
enum { M4V_MAGIC = 7, M4V_AT_VERSION = M4V_MAGIC, M4V_AT_LEVEL, M4V_HEAD };  // the header's bytes
static const U8 magic[M4V_MAGIC] = { 'm', '4', 'v', 'p', 'a', 'c', 'k' };
constexpr int M4V_VERSION = 4;  // the format's

// a compression's statistics (its summary line)
struct Stats {
  U32 vops, packed, by_type[PT_N], raw_unsup, raw_long, raw_parse, raw_verify;
  double in_bytes, raw_bytes, vop_bytes;
  U64 parts;    // -t: the parts
  int threads;  // and the threads

  void add(const Stats& s) {  // a part's VOPs and bytes (not in_bytes: the parts have headers copied in)
    int i;
    vops += s.vops;
    packed += s.packed;
    for (i = 0; i < PT_N; i++) by_type[i] += s.by_type[i];
    raw_unsup += s.raw_unsup;
    raw_long += s.raw_long;
    raw_parse += s.raw_parse;
    raw_verify += s.raw_verify;
    raw_bytes += s.raw_bytes;
    vop_bytes += s.vop_bytes;
  }

  // c's summary line (out_bytes: the compressed size, at the level), and with -s the bits by kind of
  // syntax element, the coder's cost and orig (CAT_*)
  void report(const char* name, const double* cost, const double* orig, double out_bytes, int level, int stats) const {
    static const char* const cat_name[NCAT] = { "raw chunks", "VOP and video packet headers, tails",
                                                 "types, flags, dquant", "coded block patterns", "motion vectors",
                                                 "intra DC", "positions", "levels", "signs", "escape modes" };
    int i;
    fprintf(stderr, "%s: %.0f -> %.0f bytes (%.2f%%, -%d); %u VOPs (I %u, P %u, B %u, S %u), %u packed", name,
            in_bytes, out_bytes, in_bytes ? PERCENT * out_bytes / in_bytes : 0.0, level, vops, by_type[PT_I],
            by_type[PT_P], by_type[PT_B], by_type[PT_S], packed);
    if (vops > packed)
      fprintf(stderr, ", %u kept as bytes (%u unsupported, %u too long, %u unparsable, %u not reproduced)",
              vops - packed, raw_unsup, raw_long, raw_parse, raw_verify);
    if (threads) fprintf(stderr, "; %.0f parts on %d threads", (double)parts, threads);
    fprintf(stderr, "\n");
    if (stats) {
      enum { NGRP = 5 };  // the groups of categories (CAT_*)
      static const char* const grp_name[NGRP] = { "headers, raw chunks", "macroblock types, cbp, dquant",
                                                  "motion vectors", "intra DC", "coefficients" };
      static const int grp_first[NGRP + 1] = { CAT_RAW, CAT_MB, CAT_MV, CAT_DC, CAT_SIG, NCAT };
      double o = 0, p = 0;
      int g;
      fprintf(stderr, "  %-33s %10s %10s\n", "bytes", "input", "packed");
      for (g = 0; g < NGRP; g++) {
        double og = 0, pg = 0;
        for (i = grp_first[g]; i < grp_first[g + 1]; i++) { og += orig[i]; pg += cost[i]; }
        fprintf(stderr, "  %-33s %10.0f %10.0f %6.2f%%\n", grp_name[g], og / BYTE_BITS, pg / BYTE_BITS,
                og ? PERCENT * pg / og : 0.0);
        if (grp_first[g + 1] - grp_first[g] > 1)
          for (i = grp_first[g]; i < grp_first[g + 1]; i++)
            fprintf(stderr, "    %-42s %10.0f\n", cat_name[i], cost[i] / BYTE_BITS);
        o += og;
        p += pg;
      }
      fprintf(stderr, "  %-33s %10.0f %10.0f %6.2f%%\n", "total", o / BYTE_BITS, p / BYTE_BITS,
              o ? PERCENT * p / o : 0.0);
    }
  }
};

// what W_VERIFY's VOP written back is compared with, as its writer flushes it (PB's flush, ctx a Cmp)
struct Cmp {
  const U8* p;
  size_t n, at;
  int differs;

  static void flush(void* ctx, const U8* q, size_t k) {
    Cmp& c = *(Cmp*)ctx;
    if (c.differs || k > c.n - c.at || memcmp(c.p + c.at, q, k)) c.differs = 1;
    else c.at += k;
  }
};

#include "m4vcoro.inc"  // M4vCoro: a run of c or d

// A run's coroutine, on pages of its own: zeroed, and taking memory only where the run touches
// them (its tables for the largest pictures). coro_new() makes it (plain data: then Init()),
// coro_free() frees it after its Quit().
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

  void write(const U8* p, size_t n) {
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
      if (i < k) { differs = 1; diff_at = pos + i; return; }
      pos += k;
      p += k;
      n -= k;
    }
  }
};

// Runs the coroutine c in the buffer protocol to its end: in() gives it its next input when it
// asks (Y_IN), out(more) takes what it put in its output buffer when that is full (Y_OUT, more:
// and gives it the next) or as it ends (Y_END, or Y_FAIL after an error: what drive() returns);
// show(code) after each yield (what it tells, Y_VIDEO; the progress line)
template <class C, class In, class Out, class Show>
uint drive(C& c, In in, Out out, Show show) {
  for (;;) {
    const uint r = c.coro_call(&c);
    if (r == Y_IN) in();
    else if (r != Y_VIDEO) out(r == Y_OUT);
    show(r);
    if (r != Y_IN && r != Y_OUT && r != Y_VIDEO) return r;
  }
}

// c on files: its input from f (the n bytes at pre read already, first), its output to s, and
// show(code) after each yield
static U8 io_bufs[2][IO_BUF];  // (the main thread's: input, output)
template <class C, class Show>
uint run_file(C& c, FILE* f, Sink& s, Show show, const U8* pre = 0, size_t n = 0) {
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


