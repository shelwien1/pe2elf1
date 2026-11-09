// dtsdpack - lossless compressor for the core of a DTS stream against its lossless audio: a .dtsc
// (dtssplit's core frames) coded given a WAV of the audio it was coded from (a DTS-HD Master Audio
// stream's XLL decode), into a .dtsd; and back.
// Usage: dtsdpack c [options] input.dtsc input.wav output.dtsd
//        dtsdpack d [options] input.dtsd input.wav output.dtsc
//   c    compress
//   d    decompress, restoring the core stream byte for byte (with the same WAV)
//   -9   (c) the model's level (the one this build has; d takes it from the file)
//   -v   (c) afterwards decompress the output and compare it with the input
//   -s   (c) input and output sizes per kind of syntax element
//   -q   no statistics, no progress line
//   -p   the progress line even where stderr is not a terminal
// The stream's input and output can be - for stdin and stdout (not with -v); the WAV is read once,
// from its start, so a pipe does too.
//
// How it works. The core stream is cut into core frames and the bytes between them. Each frame is
// parsed into its record (syntax.inc: dts2tsv's walk of the core, down to every subband sample's
// quantization index), written back as bits and compared with the frame: if that reproduces it,
// the record is coded with a binary arithmetic coder, else the frame is kept as bytes, and so are
// the bytes between frames, so compression is always lossless. A frame's extension in the core
// substream is in its record too: XCH's (6.1's back centre) and XXCH's channels (7.1's) after the
// core's, X96's subbands (96 kHz) in its own syntax. The same walk that reads a frame's bits and
// writes them is, at record level, what codes and decodes the record (model.inc: the model is its
// IO).
//
// The WAV. The core was coded from the audio a DTS-HD MA stream's lossless extension decodes to:
// that audio's subband samples, by the encoder's analysis filterbank (anal.inc, in integers), and
// the core's own step sizes, scale factors and ADPCM predictions, give nearly every quantization
// index the core has, and their RMS nearly every scale factor; the encoder's searches of its code
// books on them, nearly every ADPCM predictor and HF VQ vector; the WAV's LFE channel decimated,
// nearly every LFE sample; and the staircases of each subband's ABITS against its energy, less the
// level the encoder's rate control sets for the frame, most bit allocations.
// A decoded (lossy) WAV's subband samples are the decoder's: they lie on the lattice of the core's
// step and scale factor, which says nearly every bit allocation, scale factor, transient and ADPCM
// predictor (the searches).
// c finds how the core's channels and blocks line up with the WAV's, each channel one of the WAV's
// or a mix of two of them (a 7.1 stream's core is a downmix: each surround its side and rear
// surround), and keeps it in the header; the model codes each element against what the audio makes
// of it. A WAV at twice the core's rate (96 kHz) is analysed by the 64-band filterbank instead: its
// lower half is the core's (a 96 kHz stream's core is), its upper half and the core's residuals
// X96's. Without a WAV that lines up, the model codes the core by itself.
//
// The model. Each element is binarised and each decision goes to one of 8 families (fam.inc, as
// m4vpack's, h264p's and ac3pack's): counters chosen by context indices, mixed in the logistic
// domain, refined by APMs, the decision then range coded (rc.inc). famvars.inc lists what the model
// gives each family; IDX/l<L>/<family>.idx declares a level's indices over those variables and their
// tunable parameters, which IDX/idx2inc.pl turns into MOD/l<L>/ (mk.sh). The families:
//   hdr  the frame's, the coding header's and the subframes' fields (XCH's too), against their last
//        values
//   sid  a subband's PMODE (against the gain of the predictor the encoder would choose), TMODE,
//        joint scale factors; and where the WAV isn't bound, its predictor, HF VQ vector, the LFE
//   abt  the bit allocations, against the bit allocation search's (a decoded WAV's) or the
//        staircase's at the subband's energy, the ABITS seen at it, and the last ones
//   scl  the scale factors, against the one the audio's RMS makes
//   hit  whether an index that isn't the audio's is the one across the rounding boundary
//   smp  the subband samples' indexes otherwise: magnitude and sign
//   vqr  the ranks of the predictors and the HF VQ vectors in the encoder's search order
//   raw  bytes kept as bytes, the bits after the subframes, the extension's other bits
// (Whether a subband sample's index is the audio's goes straight to the coder: model_smp.inc.)
//
// File format: "dtsdpack", the format version 12, the level, the WAV's binding (how the core lines
// up with it, and what the WAV was: coro.inc), then the range coded chunks (kind: end, a frame as
// records, bytes between frames, a frame kept as bytes; then the records or a length and the
// bytes); after the end, the CRC-32 of the core stream.
//
// The source is ac3pack's framework: its families, coder, IDX and runs (Lib3's coroutines), with
// this format's syntax, analysis and model. The tables of tables.inc are FFmpeg's, so dtsdpack is
// licensed under the GNU Lesser General Public License, version 2.1 or later.
//
// Build: ./mk.sh release, or
//   g++ -O3 -march=x86-64-v3 -ffp-contract=off -std=c++11 -ILib3 -pthread -o dtsdpack dtsdpack.cpp
//   x86_64-w64-mingw32-g++ -O3 -s -static -std=c++11 -ILib3 -o dtsdpack.exe dtsdpack.cpp
// (AVX2 or not, the files are the same.) C++11 without exceptions or RTTI (GCC's or Clang's
// __int128 for the analysis); integer arithmetic wherever it decides what is coded, so files are
// portable between platforms.

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
#include <pthread.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>
#endif
#if defined(__AVX2__)
#include <immintrin.h>
#elif defined(__SSE2__)
#include <emmintrin.h>
#endif

// Lib3's stack-copying coroutines (after the system headers, which Lib3's setjmp and longjmp must
// not reach)
#include "Lib3/common.inc"
#include "Lib3/coro3b.inc"
#include "base.inc"
#include "mem.inc"

// the syntax, the streams, the WAV
#include "crc.inc"
#include "syntax.inc"
#include "io.inc"
#include "wav.inc"
// the coder and the families of the model, and the levels (each its families as IDX/l<L>
// declares them, generated into MOD/l<L>)
#include "levels.inc"
// the analysis, the reader, the model, the runs
#include "anal.inc"
#include "reader.inc"
#include "model.inc"
#include "sys.inc"
#include "lock.inc"
#include "audioinfo.inc"
#include "progress.inc"
#include "coro.inc"

// --- the command line --------------------------------------------------------------------------

struct Opts {
  int quiet, verify, stats;
  int progress;  // -p: the progress line even where stderr is not a terminal (and with -q)
  int level;     // the model's
};

// a call's file, "-" for stdin or stdout
static FILE* open_file(const char* name, int out) {
  if (!strcmp(name, "-")) return out ? stdout : stdin;
  return fopen(name, out ? "wb" : "rb");
}

// what a run did, for its progress line's end: the stream's bytes (raw), the compressed stream's
// (packed), the frames
struct RunCounts {
  U64 raw, packed, frames;
};

// a run's counts as it goes (C: DtsCoro): what its progress line shows
template <class C>
static void coro_counts(void* p, U64& raw, U64& packed, U64& pos, U64& frames) {
  const C& c = *(const C*)p;
  raw = c.raw_done();
  packed = c.packed_done();
  pos = C::f_dec ? packed : raw;
  frames = c.nframe;
}
// a run's show(): what it tells, and the progress line as it goes
template <class C>
static void run_show(Progress& pr, const C& c, uint code) {
  if (code == Y_AUDIO) pr.audio(c.audio);
  pr.poll();
}

#include "call.inc"  // Call: a call's files, its progress line, its run; d and c on it

// c -v: d of the output (with the WAV again), compared with the input (its own progress line, ver)
static int verify(FILE* in, const char* in_name, const char* wav_name, const char* out_name, const Opts& o) {
  Call k;
  FILE* t = fopen(out_name, "rb");
  FILE* w = fopen(wav_name, "rb");
  int r;
  if (!t || !w) {
    fprintf(stderr, "dtsdpack: can't open %s again\n", !t ? out_name : wav_name);
    if (t) fclose(t);
    if (w) fclose(w);
    return E_READ;
  }
  rewind(in);
  k.init(t, out_name, w, wav_name, in, in_name, 1, o, "ver");
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

static int compress(const char* in_name, const char* wav_name, const char* out_name, const Opts& o) {
  Call k;
  Stats st;
  double cost[NCAT], orig[NCAT];
  int r = k.open(in_name, wav_name, out_name, o, "enc");
  char bound[192];
  if (r) return r;
  r = k.compress_one(o, st, cost, orig);
  // how the core lined up with the WAV (the summary's)
  if (!k.wav.ok)
    snprintf(bound, sizeof bound, "the WAV not used (%s)", k.wav.why);
  else {
    int nb = 0, nm = 0, ng = 0;
    for (int ch = 0; ch < NCH; ch++)
      if (k.bind.map[ch] >= 0) nb++, nm += k.bind.map2[ch] >= 0, ng += k.bind.map2[ch] < 0 && k.bind.g1[ch] != BIND_ONE;
    char mix[64] = "";
    if (nm || ng) snprintf(mix, sizeof mix, "; %d of them mixes of two, %d scaled", nm, ng);
    if (!nb) snprintf(bound, sizeof bound, "the WAV not used (nothing lines up: correlation %.3f)", k.corr);
    else
      snprintf(bound, sizeof bound, "%d channel%s%s against the WAV%s (offset %d, correlation %.4f%s)", nb,
               nb > 1 ? "s" : "", k.bind.lfe >= 0 ? " and the LFE" : "",
               k.bind.up == 2 ? "'s 64-band analysis" : "", (int)k.bind.off, k.corr, mix);
  }
  r = k.close(r);
  if (!r && !o.quiet) st.report(in_name, cost, orig, (double)k.s.pos, o.level, o.stats, bound);
  if (!r && !o.quiet && o.stats && k.wav.ok) {  // (-s: the binding: each core channel's WAV channels, and gains not 1)
    int any = 0;
    for (int ch = 0; ch < NCH; ch++) {
      if (k.bind.map[ch] < 0) continue;
      fprintf(stderr, "%s core channel %d = ", any++ ? ";" : "  the binding:", ch);
      if (k.bind.g1[ch] != BIND_ONE || k.bind.map2[ch] >= 0) fprintf(stderr, "%.6f ", (double)k.bind.g1[ch] / BIND_ONE);
      fprintf(stderr, "WAV %d", k.bind.map[ch]);
      if (k.bind.map2[ch] >= 0) fprintf(stderr, " + %.6f WAV %d", (double)k.bind.g2[ch] / BIND_ONE, k.bind.map2[ch]);
    }
    if (k.bind.lfe >= 0) fprintf(stderr, "%s LFE = WAV %d", any++ ? ";" : "  the binding:", k.bind.lfe);
    if (any) fprintf(stderr, "\n");
  }
  if (!r && o.verify) r = verify(k.in, in_name, wav_name, out_name, o);
  if (k.in != stdin) fclose(k.in);
  return r;
}

static int decompress(const char* in_name, const char* wav_name, const char* out_name, const Opts& o) {
  Call k;
  int r = k.open(in_name, wav_name, out_name, o, "dec");
  if (r) return r;
  r = k.close(k.decode_to());
  if (k.in != stdin) fclose(k.in);
  return r;
}

static void usage(void) {
  char lv[NOTE_MAX];
  int k = 0;
  size_t i;
  for (i = 0; i < sizeof kLevels / sizeof kLevels[0]; i++) k += snprintf(lv + k, sizeof lv - k, " -%d", kLevels[i]);
  fprintf(stderr, "dtsdpack - lossless compressor for a DTS core stream against its lossless audio\n"
                  "Usage: dtsdpack c [options] input.dtsc input.wav output.dtsd\n"
                  "       dtsdpack d [options] input.dtsd input.wav output.dtsc\n"
                  "  c     compress the core frames (dtssplit's .dtsc, or any DTS core stream), given\n"
                  "        the WAV of the audio they were coded from (a DTS-HD MA stream's lossless\n"
                  "        decode: ffmpeg -i in.dts -c:a pcm_s24le in.wav)\n"
                  "  d     decompress, with the same WAV\n"
                  "  -v    (c) decompress the result again and compare it with the input\n"
                  "  -s    (c) bytes spent per kind of syntax element\n"
                  "  -q    no statistics, no progress line\n"
                  "  -p    the progress line even where stderr is not a terminal\n"
                  "  %s  (c) the model's level; d takes it from the stream\n"
                  "the core stream's input and output can be - for stdin and stdout (not with -v)\n",
          lv + 1);
}

int main(int argc, char** argv) {
  Opts o;
  int a;
  memset(&o, 0, sizeof(o));
  o.level = kLevels[0];
  huff_init();
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
        fprintf(stderr, "dtsdpack: no level -%d in this build\n", o.level);
        return E_USAGE;
      }
    } else {
      usage();
      return E_USAGE;
    }
  }
  if (a != argc - 3) {  // (input, wav, output)
    usage();
    return E_USAGE;
  }
  if (!strcmp(argv[a + 1], "-") && (!strcmp(argv[a], "-"))) {
    fprintf(stderr, "dtsdpack: the stream and the WAV can't both be stdin\n");
    return E_USAGE;
  }
  if (o.verify && (!strcmp(argv[a], "-") || !strcmp(argv[a + 1], "-") || !strcmp(argv[a + 2], "-"))) {
    fprintf(stderr, "dtsdpack: -v reads the input, the WAV and the output again: they can't be stdin or stdout\n");
    return E_USAGE;
  }
#ifdef _WIN32
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  return argv[1][0] == 'c' ? compress(argv[a], argv[a + 1], argv[a + 2], o) : decompress(argv[a], argv[a + 1], argv[a + 2], o);
}
