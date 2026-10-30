// m4vpack - lossless compressor for MPEG-4 Part 2 video elementary streams (.m4v: XviD, DivX 4/5/6,
// libavcodec mpeg4, ...)
// Usage: m4vpack c|d [options] input output
//   c    compress
//   d    decompress, restoring the original stream byte for byte
//   -9 -7 -5 -3 -1  (c) the model's level: -9 (the default) the strongest, -1 the fastest
//   -v   (c) afterwards decompress the output and compare it with the input
//   -s   (c) input and output sizes per kind of syntax element
//   -q   no statistics, no progress line
//   -p   the progress line even where stderr is not a terminal (as h264p's: progress.inc)
//   -t N (c) compress on N threads (1 to 64): the stream in parts, a model for each
//   -c N (c with -t) the size of a part, at least: N MB, or N KB with a k (default 4 MB)
// input and output can be - for stdin and stdout.
//
// The bitstream syntax and the reconstruction of level -9 follow m4vdec.cpp, a port of FFmpeg's
// MPEG-4 Part 2 decoder, so m4vpack is licensed under the GNU Lesser General Public License 2.1 or
// later.
//
// How it works. The stream is cut at start codes (00 00 01 xx). Each VOP is parsed into the
// syntax records m4vtsv writes as text (VOP and video packet headers, one record per macroblock
// with its motion vector differences and coefficient events, and the bits after the last
// macroblock), and the records are written back as bits and compared with the input. If that
// reproduces the VOP, the records are coded with a binary arithmetic coder; otherwise (unsupported
// features, damaged data) the VOP is kept as bytes, so compression is always lossless. Headers and
// user data are kept as bytes too. The same functions code and decode every record (model.inc), so
// the two directions can't disagree.
//
// The model. Each syntax element is binarised (as H.264's CABAC; see m4vtsv/h264-counterparts.md),
// and each decision goes to one of 13 families (fam.inc, h264p's): counters chosen by context
// indices, mixed in the logistic domain, refined by APMs. famvars.inc lists what the model gives
// each family; IDX/l<L>/<family>.idx declares a level's indices over those variables and their
// tunable parameters, which IDX/idx2inc.pl turns into MOD/l<L>/ (mk.sh). The levels (levels.inc,
// lv<L>.inc) differ in how much each family mixes:
//   -9  ten counters, three mixers and a final one, two APMs, and the decoder in the loop
//   -7  the same without the decoder
//   -5  ten counters, one mixer
//   -3  six counters, one mixer
//   -1  four counters, one mixer
// Level -9 reconstructs the pictures as it codes them (recon.inc: m4vdec's decoder, driven by the
// records), so its contexts can look at the samples: the first signs of a block predicted by how
// well the block continues the decoded samples above and left of it; an intra macroblock's
// coefficients estimated from the reference (the block its neighbours' motion points at, refined
// by template matching, transformed and quantised); an inter block's prediction's texture and how
// far its edges are from the decoded samples; the levels those edges ask for; how much the area
// changed since the reference, for skipping; the vector the template matches best, for motion
// vector differences.
//
// Streaming. The compressor and the decompressor are coroutines (Lib3's, as h264p's): each takes
// its input and gives its output a buffer at a time, so they run on files, pipes or memory alike,
// and any number of them at once. Memory grows with the longest chunk and the largest picture.
//
// Threads (-t). The stream is cut into parts that start at a VOP header (VOS, VO, VOL) or an
// I-VOP, with the headers before them copied in, so that each part is a stream of its own; the
// parts are dealt in turn to N workers (MTwrap), each part coded by a model of its own.
//
// File format: "m4vpack", the format version 04 and the level, then the arithmetic coded chunks:
// kind (end, VOP as records, bytes), then the records or a length and the bytes; after the end, the
// CRC-32 of the stream. On threads: "m4vpm" 04, the threads, the part size in KB (4 bytes), then
// MTwrap's container of the workers' streams, a record for each part (its size, where the headers
// copied in are, its place, a CRC), each part as a stream of the first kind (with its level).
//
// Kept as bytes: as in m4vtsv, VOPs using data partitioning (B-VOPs don't), non-rectangular
// shapes, static sprites, sprite brightness change, newpred, reduced resolution, scalability or the
// studio profile; VOPs that don't parse (damaged data); VOPs longer than 64 MiB or with more than
// 1 MiB after the last macroblock.
//
// The source: this file includes the parts in order (below); each class (but cm.inc's small
// counters), and each function over 100 lines, is in a file of its own named after it
// (m4vcoro.inc, code_block.inc, ...), which the file of its part includes where it belongs.
//
// Build (Lib3/, MTwrap_v3/ and MOD/ next to this file): ./mk.sh release, or
//   g++ -O3 -march=x86-64-v3 -ffp-contract=off -std=c++11 -ILib3 -pthread -o m4vpack m4vpack.cpp
//   x86_64-w64-mingw32-g++ -O3 -s -static -std=c++11 -ILib3 -o m4vpack.exe m4vpack.cpp
// (AVX2 or not, the files are the same.) C++11 without exceptions or RTTI; integer arithmetic
// wherever it decides what is coded, so files are portable between platforms.

#ifndef _WIN32
#define _FILE_OFFSET_BITS 64
#endif
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>
#endif
#if defined(__AVX2__)
#include <immintrin.h>
#elif defined(__SSE2__)
#include <emmintrin.h>
#endif

// Lib3's stack-copying coroutines and its CoroutinePair, which chains two of them; then MTwrap,
// which runs coroutines on threads (its win_shim.inc: build with -ILib3). After the system headers,
// which Lib3's setjmp and longjmp must not reach.
#include "Lib3/common.inc"
#include "Lib3/coro3b.inc"
#include "Lib3/coro2pair.inc"
#include "base.inc"
#include "mem.inc"
#define MTWRAP_NOMEM() no_memory()  // (MTwrap's buffers)
#include "MTwrap_v3/MTwrap.inc"

// the syntax and the streams
#include "syntax.inc"
#include "io.inc"
#include "reader.inc"
#include "crc.inc"
// the coder and the families of the model, and the levels (each its families as IDX/l<L>
// declares them, generated into MOD/l<L>)
#include "levels.inc"
// the model, the runs (coroutines) and the threads
#include "recon.inc"
#include "reconctx.inc"
#include "modelbase.inc"
#include "model.inc"
#include "sys.inc"
#include "lock.inc"
#include "videoinfo.inc"
#include "progress.inc"
#include "coro.inc"
#include "mt.inc"

// --- the command line --------------------------------------------------------------------------

typedef struct Opts {
  int quiet, verify, stats;
  int progress;  // -p: the progress line even where stderr is not a terminal (and with -q)
  int level;     // the model's (-1 to -9)
  int threads;   // -t
  U32 part;      // -c: a part's size, at least
} Opts;

// a call's file, "-" for stdin or stdout
static FILE* open_file(const char* name, int out) {
  if (!strcmp(name, "-")) return out ? stdout : stdin;
  return fopen(name, out ? "wb" : "rb");
}

// the kinds of syntax element, as -s names them (coder.inc's CAT_*)
static const char* cat_name[NCAT] = { "raw chunks", "VOP and video packet headers, tails", "types, flags, dquant",
                                      "coded block patterns", "motion vectors", "intra DC", "positions",
                                      "levels", "signs", "escape modes" };

// c's summary line (out_bytes: the compressed size, at the level), and with -s the bits by kind of
// syntax element
static void report(const char* name, const Stats* st, const double* cost, const double* orig, double out_bytes,
                   int level, int stats) {
  int i;
  fprintf(stderr, "%s: %.0f -> %.0f bytes (%.2f%%, -%d); %u VOPs (I %u, P %u, B %u, S %u), %u packed", name,
          st->in_bytes, out_bytes, st->in_bytes ? PERCENT * out_bytes / st->in_bytes : 0.0, level, st->vops,
          st->by_type[PT_I], st->by_type[PT_P], st->by_type[PT_B], st->by_type[PT_S], st->packed);
  if (st->vops > st->packed)
    fprintf(stderr, ", %u kept as bytes (%u unsupported, %u too long, %u unparsable, %u not reproduced)",
            st->vops - st->packed, st->raw_unsup, st->raw_long, st->raw_parse, st->raw_verify);
  if (st->threads) fprintf(stderr, "; %.0f parts on %d threads", (double)st->parts, st->threads);
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

// --- a call: its files, its progress line, its run ---------------------------------------------

// what a run did, for its progress line's end: the stream's bytes (raw), the compressed stream's
// (packed), the VOPs
typedef struct RunCounts { U64 raw, packed, vops; } RunCounts;

// A call's input (in, named in_name), its output (the sink s, named out_name: a file, or for -v's
// check the input to compare with), its progress line and what its run did
typedef struct Call {
  FILE* in;
  const char* in_name;
  Sink s;
  const char* out_name;
  Progress pr;
  RunCounts n;
} Call;

// the call k from the file in to out (compare: s compares with out instead of writing it), its
// progress line tagged tag: shown with -p, or where stderr is a terminal, but not with -q
static void call_init(Call* k, FILE* in, const char* in_name, FILE* out, const char* out_name, int compare,
                      const Opts* o, const char* tag) {
  k->in = in;
  k->in_name = in_name;
  memset(&k->s, 0, sizeof k->s);
  k->s.f = out;
  k->s.compare = compare;
  k->out_name = out_name;
  memset(&k->n, 0, sizeof k->n);
  k->pr.Init();
  if (o->progress || (!o->quiet && stderr_tty())) k->pr.start(tag, file_size(in));
}
// c's or d's call from the file in_name to out_name: 0, or the exit status of an error (said)
static int call_open(Call* k, const char* in_name, const char* out_name, const Opts* o, const char* tag) {
  FILE *in, *out;
  if (!(in = open_file(in_name, 0))) {
    fprintf(stderr, "m4vpack: can't open %s\n", in_name);
    return E_READ;
  }
  if (!(out = open_file(out_name, 1))) {
    fprintf(stderr, "m4vpack: can't create %s\n", out_name);
    if (in != stdin) fclose(in);
    return E_WRITE;
  }
  call_init(k, in, in_name, out, out_name, 0, o, tag);
  return 0;
}
// the end of the call's progress line, its run's status r
static void call_end(Call* k, int r) {
  k->pr.end(r, k->n.raw, k->n.packed, k->n.vops);
  k->pr.Quit();
}
// the end of c's or d's call, its run's status r: the output flushed and closed (the input stays
// open: -v reads it again), the progress line ended; r, or the exit status of an error writing
// (said)
static int call_close(Call* k, int r) {
  const int flushed = fflush(k->s.f) == 0, closed = k->s.f == stdout || fclose(k->s.f) == 0;
  if (!(flushed && closed) && !r) r = (say_error(&k->pr, "m4vpack: %s: write error\n", k->out_name), E_WRITE);
  call_end(k, r);
  return r;
}

// a run's counts as it goes (C: M4vCoro): what its progress line shows
template <class C>
static void coro_counts(void* p, U64& raw, U64& packed, U64& pos, U64& vops) {
  const C& c = *(const C*)p;
  raw = c.raw_done();
  packed = c.packed_done();
  pos = C::f_dec ? packed : raw;
  vops = c.nvop;
}
// and those of a run on threads: its workers', added up (M: MTwrap's), with the header before them
template <class M>
static void mt_sums(const M& m, U64& raw, U64& packed, U64& vops) {
  uint i;
  raw = vops = 0;
  packed = MT_HEAD;
  for (i = 0; i < m.th_N; i++) m.C1[i].prog_get(raw, packed, vops);
}
template <class M, int f_DEC>
static void mt_counts(void* p, U64& raw, U64& packed, U64& pos, U64& vops) {
  mt_sums(*(const M*)p, raw, packed, vops);
  pos = f_DEC ? packed : raw;
}
// a run's show(): what it tells, and the progress line as it goes
template <class C>
static void run_show(Progress& pr, const C* c, uint code) {
  if (code == Y_VIDEO) pr.video(c->video);
  pr.poll();
}

// The run c (a coroutine) on the call's files, the bytes at head (got of them) read already first:
// its progress line's counts f(ctx) as it goes, show(code) after each yield; what run_file() returns
template <class C, class Show>
static uint run_call(Call* k, C& c, ProgressCounts f, void* ctx, Show show, const U8* head = 0, size_t got = 0) {
  uint y;
  k->pr.counts(f, ctx);
  y = run_file(c, k->in, &k->s, show, head, got);
  k->pr.counts(nullptr, nullptr);
  return y;
}
// the end of a run on the call's files: its status r (0, or the exit status of an error the run's
// note says), unless reading or writing failed; said after the progress line
static int run_end(Call* k, int r, const char* note) {
  if (ferror(k->in)) {
    say_error(&k->pr, "m4vpack: %s: read error\n", k->in_name);
    return E_READ;
  }
  if (k->s.err) {
    say_error(&k->pr, "m4vpack: %s: write error\n", k->out_name);
    return E_WRITE;
  }
  if (r) say_error(&k->pr, "m4vpack: %s: %s\n", k->in_name, note);
  return r;
}
// a run of one model (C: M4vCoro) on the call's files: its counts into k->n, its status (said)
template <class C>
static int run_one(Call* k, C* c, const U8* head, size_t got) {
  run_call(k, *c, coro_counts<C>, c, [&](uint code) { run_show(k->pr, c, code); }, head, got);
  k->n.raw = c->raw_done();
  k->n.packed = c->packed_done();
  k->n.vops = c->nvop;
  return run_end(k, c->status, c->note);
}

// what MTwrap's Init() returned, said (0: nothing to say)
static int mt_init_failed(uint r, int threads, Progress* pr) {
  if (r == MTW_NO_THREAD) say_error(pr, "m4vpack: can't start %d threads\n", threads);
  else if (r) say_error(pr, "m4vpack: no memory for the buffers of %d threads\n", threads);
  return r != 0;
}

// --- d -----------------------------------------------------------------------------------------

// d of a stream of parts (the bytes at head, got of them, read already)
static int decode_mt(Call* k, const U8* head, size_t got) {
  const int threads = got >= MT_HEAD ? head[MT_AT_THREADS] : 0;
  char note[NOTE_MAX] = "damaged data";
  MtArgs a;
  MtDec* d;
  uint y;
  int r;
  if (got > MT_AT_VERSION && head[MT_AT_VERSION] != M4V_VERSION) {
    say_error(&k->pr, "m4vpack: %s: format version %d, which this m4vpack (version %d) doesn't read\n", k->in_name,
              head[MT_AT_VERSION], M4V_VERSION);
    return E_DATA;
  }
  if (threads < 1 || threads > MAX_THREADS) return run_end(k, E_DATA, note);
  a.threads = threads;
  a.stats = 0;
  a.level = 0;
  a.part_max = mt_part_max((get_le(head + MT_AT_PART, MT_PART_BYTES) + 1) * KIB);  // (the part size in KB, rounded down)
  a.meter = &k->pr;
  if (!(d = coro_new<MtDec>())) no_memory();
  if (mt_init_failed(d->Init(threads, MT_RING, &a, 1), threads, &k->pr)) return E_INTERNAL;
  y = run_call(k, *d, mt_counts<MtDec, 1>, d, [&](uint) { k->pr.poll(); }, head + MT_HEAD, got - MT_HEAD);
  if (!(r = mt_failed(*d, note, sizeof note)) && (y != Y_END || !mt_dealt(*d))) r = E_DATA;
  mt_sums(*d, k->n.raw, k->n.packed, k->n.vops);
  k->n.packed = d->c_inp + MT_HEAD;
  d->Quit();  // (the threads joined)
  coro_free(d);
  return run_end(k, r, note);
}

#include "decodeone.inc"  // DecodeOne: d of one model's stream

// d of the call's input into its output: 0, or the exit status of an error (said). Its first bytes
// say which it is, a stream of one model (and its level) or of parts on threads (mt_magic).
static int decode_to(Call* k) {
  U8 head[HEAD_MAX];
  const size_t got = fread(head, 1, sizeof head, k->in);
  int r;
  memset(&k->n, 0, sizeof k->n);
  if (got >= sizeof mt_magic && !memcmp(head, mt_magic, sizeof mt_magic)) return decode_mt(k, head, got);
  if (got < M4V_HEAD || memcmp(head, magic, sizeof magic)) {
    if (ferror(k->in)) return run_end(k, 0, "");
    say_error(&k->pr, "m4vpack: %s: not an m4vpack file\n", k->in_name);
    return E_DATA;
  }
  if (head[M4V_AT_VERSION] != M4V_VERSION) {
    say_error(&k->pr, "m4vpack: %s: format version %d, which this m4vpack (version %d) doesn't read\n", k->in_name,
              head[M4V_AT_VERSION], M4V_VERSION);
    return E_DATA;
  }
  r = with_level<DecodeOne>(head[M4V_AT_LEVEL], k, (const U8*)head, got);
  if (r != kNoLevel) return r;
  say_error(&k->pr, "m4vpack: %s: compressed at level -%d, which this build has not\n", k->in_name,
            head[M4V_AT_LEVEL]);
  return E_DATA;
}

// c -v: d of the output, compared with the input (its own progress line, ver)
static int verify(FILE* in, const char* in_name, const char* out_name, const Opts* o) {
  Call k;
  FILE* t = fopen(out_name, "rb");
  int r;
  if (!t) {
    fprintf(stderr, "m4vpack: can't open %s\n", out_name);
    return E_WRITE;
  }
  rewind(in);
  call_init(&k, t, out_name, in, in_name, 1, o, "ver");
  r = decode_to(&k);
  fclose(t);
  if (!r && !k.s.differs && fgetc(in) != EOF) {
    k.s.differs = 1;
    k.s.diff_at = k.s.pos;
  }
  if (r || k.s.differs) {
    say_error(&k.pr, "verify: FAILED (%s at byte %.0f)\n", r ? "decoding error" : "difference", (double)k.s.diff_at);
    r = E_INTERNAL;
  }
  call_end(&k, r);
  if (!r && !o->quiet) fprintf(stderr, "verify: OK\n");
  return r;
}

// --- c -----------------------------------------------------------------------------------------

#include "compressone.inc"  // CompressOne: c on one model
static int compress_one(Call* k, const Opts* o, Stats* st, double* cost, double* orig) {
  return with_level<CompressOne>(o->level, k, o, st, cost, orig);
}

// c -t N: the stream in parts on N threads, its statistics the parts' added up
static int compress_mt(Call* k, const Opts* o, Stats* st, double* cost, double* orig) {
  char note[NOTE_MAX] = "internal error: a thread stopped early";
  U8 head[MT_HEAD];
  MtArgs a;
  MtEnc* e;
  uint y, i;
  int r, j;
  a.threads = o->threads;
  a.stats = o->stats;
  a.level = o->level;
  a.part_max = 0;
  a.meter = &k->pr;
  if (!(e = coro_new<MtEnc>())) no_memory();
  e->coro_init();
  e->M1.Init(o->part);
  if (mt_init_failed(e->M2.Init(o->threads, MT_RING, &a, 1), o->threads, &k->pr))
    return E_INTERNAL;  // (started threads wait: they end with the process)
  mt_head(head, o->threads, o->part);
  sink_write(&k->s, head, sizeof head);
  y = run_call(k, *e, mt_counts<MtEncWorkers, 0>, &e->M2, [&](uint) { k->pr.poll(); });
  if (!(r = mt_failed(e->M2, note, sizeof note)) && y != Y_END) r = E_INTERNAL;
  memset(st, 0, sizeof *st);
  memset(cost, 0, NCAT * sizeof *cost);
  memset(orig, 0, NCAT * sizeof *orig);
  for (i = 0; i < e->M2.th_N; i++) {
    const M4vPart<0>& w = e->M2.C1[i];
    st->vops += w.st.vops;
    st->packed += w.st.packed;
    for (j = 0; j < PT_N; j++) st->by_type[j] += w.st.by_type[j];
    st->raw_unsup += w.st.raw_unsup;
    st->raw_long += w.st.raw_long;
    st->raw_parse += w.st.raw_parse;
    st->raw_verify += w.st.raw_verify;
    st->raw_bytes += w.st.raw_bytes;
    st->vop_bytes += w.st.vop_bytes;
    for (j = 0; j < NCAT; j++) cost[j] += w.cost[j], orig[j] += w.orig[j];
  }
  st->in_bytes = (double)e->M1.total;  // (the parts have the headers copied in)
  st->parts = e->M1.parts;
  st->threads = o->threads;
  mt_sums(e->M2, k->n.raw, k->n.packed, k->n.vops);
  k->n.raw = e->M1.total;
  k->n.packed = k->s.pos;
  e->M2.Quit();  // (the threads joined, the workers freed)
  e->M1.Quit();
  coro_free(e);
  return run_end(k, r, note);
}

static int compress(const char* in_name, const char* out_name, const Opts* o) {
  Call k;
  Stats st;
  double cost[NCAT], orig[NCAT];
  int r = call_open(&k, in_name, out_name, o, "enc");
  if (r) return r;
  r = call_close(&k, (o->threads > 1 ? compress_mt : compress_one)(&k, o, &st, cost, orig));
  if (!r && !o->quiet) report(in_name, &st, cost, orig, (double)k.s.pos, o->level, o->stats);
  if (!r && o->verify) r = verify(k.in, in_name, out_name, o);
  if (k.in != stdin) fclose(k.in);
  return r;
}

static int decompress(const char* in_name, const char* out_name, const Opts* o) {
  Call k;
  int r = call_open(&k, in_name, out_name, o, "dec");
  if (r) return r;
  r = call_close(&k, decode_to(&k));
  if (k.in != stdin) fclose(k.in);
  return r;
}

// -c: a part's size, at least (MB, or KB with a k)
constexpr U32 PART_DEFAULT_MB = 4, PART_MAX_MB = 1024;

static void usage(void) {
  char lv[NOTE_MAX];
  int k = 0;
  size_t i;
  for (i = 0; i < sizeof kLevels / sizeof kLevels[0]; i++) k += snprintf(lv + k, sizeof lv - k, " -%d", kLevels[i]);
  fprintf(stderr, "m4vpack - lossless compressor for MPEG-4 Part 2 video streams (XviD, DivX 4+)\n"
                  "Usage: m4vpack c|d [options] input output\n"
                  "  c     compress a raw .m4v stream\n"
                  "  d     decompress\n"
                  "  -v    (c) decompress the result again and compare it with the input\n"
                  "  -s    (c) bits spent per kind of syntax element\n"
                  "  -q    no statistics, no progress line\n"
                  "  -p    the progress line even where stderr is not a terminal\n"
                  "  %s  (c) the model's level, from the strongest (the default) to the fastest;\n"
                  "        d takes it from the stream\n"
                  "  -t N  (c) compress on N threads, 1 to %d (default 1): the stream in parts that\n"
                  "        start at I-VOPs (where there are none, at others past %d times the part\n"
                  "        size), a model for each; d takes N from the stream\n"
                  "  -c N  (c with -t) the size of a part, at least: N MB (1 to %u), or N KB with\n"
                  "        a k (default %u)\n"
                  "input and output can be - for stdin and stdout (not with -v)\n",
          lv + 1, MAX_THREADS, (int)MT_CAP_PARTS, PART_MAX_MB, PART_DEFAULT_MB);
}

int main(int argc, char** argv) {
  Opts o;
  int a;
  memset(&o, 0, sizeof(o));
  o.threads = 1;
  o.part = PART_DEFAULT_MB * MIB;
  o.level = kLevels[0];
  if (argc < 2 || (strcmp(argv[1], "c") && strcmp(argv[1], "d"))) {
    usage();
    return E_USAGE;
  }
  for (a = 2; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
    if (!strcmp(argv[a], "-q")) o.quiet = 1;
    else if (!strcmp(argv[a], "-p")) o.progress = 1;
    else if (!strcmp(argv[a], "-v")) o.verify = 1;
    else if (!strcmp(argv[a], "-s")) o.stats = 1;
    else if (argv[a][1] >= '1' && argv[a][1] <= '9' && !argv[a][2]) {
      size_t i;
      o.level = argv[a][1] - '0';
      for (i = 0; i < sizeof kLevels / sizeof kLevels[0] && kLevels[i] != o.level; i++) {}
      if (i == sizeof kLevels / sizeof kLevels[0]) {
        fprintf(stderr, "m4vpack: no level -%d in this build\n", o.level);
        return E_USAGE;
      }
    }
    else if ((argv[a][1] == 't' || argv[a][1] == 'c') && (argv[a][2] || a + 1 < argc)) {
      // -t N, -c N (or -tN, -cN)
      const char opt = argv[a][1];
      const char* v = argv[a][2] ? argv[a] + 2 : argv[++a];
      char* e;
      constexpr int DECIMAL = 10;  // (strtoul()'s base)
      const unsigned long n = strtoul(v, &e, DECIMAL);
      if (opt == 't') {
        if (*e || e == v || n < 1 || n > MAX_THREADS) {
          fprintf(stderr, "m4vpack: -t takes 1 to %d threads\n", MAX_THREADS);
          return E_USAGE;
        }
        o.threads = (int)n;
      } else {
        unsigned long unit = MIB;
        if (*e == 'k' || *e == 'K') unit = KIB, e++;
        if (*e || e == v || n < 1 || n > (unsigned long)PART_MAX_MB * MIB / unit) {
          fprintf(stderr, "m4vpack: -c takes a part size of 1 to %u MB, or in KB with a k\n", PART_MAX_MB);
          return E_USAGE;
        }
        o.part = (U32)(n * unit);
      }
    } else {
      usage();
      return E_USAGE;
    }
  }
  if (a != argc - 2) {  // (input, output)
    usage();
    return E_USAGE;
  }
  if (o.verify && (!strcmp(argv[a], "-") || !strcmp(argv[a + 1], "-"))) {
    fprintf(stderr, "m4vpack: -v reads the input and the output again: they can't be stdin or stdout\n");
    return E_USAGE;
  }
#ifdef _WIN32
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  return argv[1][0] == 'c' ? compress(argv[a], argv[a + 1], &o) : decompress(argv[a], argv[a + 1], &o);
}
