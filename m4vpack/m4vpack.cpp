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
// The bitstream syntax, the vectors and the reconstruction of level -9 follow m4vdec.cpp, a port of
// FFmpeg's MPEG-4 Part 2 decoder, so m4vpack is licensed under the GNU Lesser General Public License
// 2.1 or later.
//
// How it works. The stream is cut at start codes (00 00 01 xx). Each VOP is parsed into the
// syntax records m4vtsv writes as text (VOP and video packet headers, one record per macroblock
// with its motion vector differences and coefficient events, and the bits after the last
// macroblock), and the records are written back as bits and compared with the input. If that
// reproduces the VOP, the records are coded with a binary arithmetic coder; otherwise (unsupported
// features, damaged data) the VOP is kept as bytes, so compression is always lossless. Headers and
// user data are kept as bytes too. The same functions code and decode every record (model.inc, by
// element: headers, residual, vectors, macroblocks), and one walk per record reads it and writes it
// back (syntax.inc), so the two directions can't disagree.
//
// The model. Each syntax element is binarised (as H.264's CABAC; see m4vtsv/h264-counterparts.md),
// and each decision goes to one of 13 families (fam.inc, h264p's): counters chosen by context
// indices, mixed in the logistic domain, refined by APMs, the decision then range coded (rc.inc,
// oggcomp's coder as h264p has it, which the run's coroutine is: coder.inc). famvars.inc lists what
// the model gives each family; IDX/l<L>/<family>.idx declares a level's indices over those
// variables and their tunable parameters, which IDX/idx2inc.pl turns into MOD/l<L>/ (mk.sh). The
// levels (levels.inc, lv<L>.inc) differ in how much each family mixes:
//   -9  ten counters, three mixers and a final one, two APMs, and the decoder in the loop
//   -7  the same without the decoder
//   -5  ten counters, one mixer
//   -3  six counters, one mixer
//   -1  four counters, one mixer
// The vectors are the decoder's at every level (motion.inc: m4vdec's vector prediction, B-VOP
// predictors, direct mode, GMC's vector and display times, one state for the model and the decoder).
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
// File format: "m4vpack", the format version 06 and the level, then the range coded chunks:
// kind (end, VOP as records, bytes), then the records or a length and the bytes; after the end, the
// CRC-32 of the stream. On threads: "m4vpm" 06, the threads, the part size in KB (4 bytes), then
// MTwrap's container of the workers' streams, interleaved as they were coded: a record for each
// part (its size, where the headers copied in are, its place, a CRC), then the part as a stream of
// the first kind (with its level), which ends where its decoder stops reading.
//
// Kept as bytes: as in m4vtsv, VOPs using data partitioning (B-VOPs don't), non-rectangular
// shapes, static sprites, sprite brightness change, newpred, reduced resolution, scalability or the
// studio profile; VOPs that don't parse (damaged data); VOPs longer than 64 MiB or with more than
// 1 MiB after the last macroblock.
//
// The source: classes with their methods defined in their bodies, and the free functions that serve
// them. This file includes the parts in order (below); each class over 100 lines, and each method
// over 100 lines, is in a file of its own named after it (m4vcoro.inc, mb.inc, code_block.inc, ...):
// a class's where its part includes it, a method's in its class's body.
//
// Build (Lib3/, MTwrap_v3/ and MOD/ next to this file): ./mk.sh release, or
//   g++ -O3 -march=x86-64-v3 -ffp-contract=off -std=c++11 -ILib3 -pthread -o m4vpack m4vpack.cpp
//   x86_64-w64-mingw32-g++ -O3 -s -static -std=c++11 -ILib3 -o m4vpack.exe m4vpack.cpp
// (AVX2 or not, the files are the same.) C++11 without exceptions or RTTI; integer arithmetic
// wherever it decides what is coded, so files are portable between platforms. -DMTWRAP_DBG builds
// it with MTwrap_v3_dbg, which writes what the threads did to dbg.txt (MTwrap_v3_dbg/mtdbg.py).

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
#ifdef MTWRAP_DBG  // MTwrap_v3_dbg instead: with statistics, written to dbg.txt as a run on threads quits
#include "MTwrap_v3_dbg/MTwrap.inc"
#else
#include "MTwrap_v3/MTwrap.inc"
#endif

// the syntax and the streams
#include "syntax.inc"
#include "io.inc"
#include "reader.inc"
#include "crc.inc"
// the coder and the families of the model, and the levels (each its families as IDX/l<L>
// declares them, generated into MOD/l<L>)
#include "levels.inc"
// the model, the runs (coroutines) and the threads
#include "motion.inc"
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

struct Opts {
  int quiet, verify, stats;
  int progress;  // -p: the progress line even where stderr is not a terminal (and with -q)
  int level;     // the model's (-1 to -9)
  int threads;   // -t
  U32 part;      // -c: a part's size, at least
};

// a call's file, "-" for stdin or stdout
static FILE* open_file(const char* name, int out) {
  if (!strcmp(name, "-")) return out ? stdout : stdin;
  return fopen(name, out ? "wb" : "rb");
}

// --- a call: its files, its progress line, its run ---------------------------------------------

// what a run did, for its progress line's end: the stream's bytes (raw), the compressed stream's
// (packed), the VOPs
struct RunCounts { U64 raw, packed, vops; };

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
static void run_show(Progress& pr, const C& c, uint code) {
  if (code == Y_VIDEO) pr.video(c.video);
  pr.poll();
}

#include "call.inc"  // Call: a call's files, its progress line, its runs; d and c on it

// c -v: d of the output, compared with the input (its own progress line, ver)
static int verify(FILE* in, const char* in_name, const char* out_name, const Opts& o) {
  Call k;
  FILE* t = fopen(out_name, "rb");
  int r;
  if (!t) {
    fprintf(stderr, "m4vpack: can't open %s\n", out_name);
    return E_WRITE;
  }
  rewind(in);
  k.init(t, out_name, in, in_name, 1, o, "ver");
  r = k.decode_to();
  fclose(t);
  if (!r && !k.s.differs && fgetc(in) != EOF) {
    k.s.differs = 1;
    k.s.diff_at = k.s.pos;
  }
  if (r || k.s.differs) {
    k.pr.say("verify: FAILED (%s at byte %.0f)\n", r ? "decoding error" : "difference", (double)k.s.diff_at);
    r = E_INTERNAL;
  }
  k.end(r);
  if (!r && !o.quiet) fprintf(stderr, "verify: OK\n");
  return r;
}

static int compress(const char* in_name, const char* out_name, const Opts& o) {
  Call k;
  Stats st;
  double cost[NCAT], orig[NCAT];
  int r = k.open(in_name, out_name, o, "enc");
  if (r) return r;
  r = k.close(o.threads > 1 ? k.compress_mt(o, st, cost, orig) : k.compress_one(o, st, cost, orig));
  if (!r && !o.quiet) st.report(in_name, cost, orig, (double)k.s.pos, o.level, o.stats);
  if (!r && o.verify) r = verify(k.in, in_name, out_name, o);
  if (k.in != stdin) fclose(k.in);
  return r;
}

static int decompress(const char* in_name, const char* out_name, const Opts& o) {
  Call k;
  int r = k.open(in_name, out_name, o, "dec");
  if (r) return r;
  r = k.close(k.decode_to());
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
  return argv[1][0] == 'c' ? compress(argv[a], argv[a + 1], o) : decompress(argv[a], argv[a + 1], o);
}
