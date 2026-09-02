#define _FILE_OFFSET_BITS 64

// Every system header the whole program needs, in one place: coder0 and the
// transformer are one translation unit, and the transformer's own sources
// (tf/*.inc) carry no #include <...> of their own.
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Intel SSE/AVX intrinsics: the denormal microcode fix below, and the
// transformer kernels (AVX2/FMA/F16C, all present in -march=haswell).
#if defined(__SSE3__)||defined(__x86_64__)||defined(_M_X64)
#include <xmmintrin.h>
#include <pmmintrin.h>   // _MM_SET_DENORMALS_ZERO_MODE (gcc puts it here)
#include <immintrin.h>
#endif
#if defined(__linux__)
#include <sys/mman.h>    // arena_build's hugepage allocation
#endif

typedef uint16_t word;
typedef uint32_t uint;
typedef uint8_t byte;
typedef uint64_t qword;

typedef float*__restrict prfloat;
typedef const float*__restrict cprfloat;

#ifdef __GNUC__
#define INLINE __attribute__((always_inline))
#define NOINLINE __attribute__((noinline))
#define ALIGN(n) __attribute__((aligned(n)))
#define restrict __restrict
#else
#define INLINE __forceinline
#define NOINLINE __declspec(noinline)
#define ALIGN(n) __declspec(align(n))
#endif

#define AlignUp(x, r) ((x)+((r)-1))/(r)*(r)

uint flen(FILE* f) {
  fseek(f, 0, SEEK_END);
  uint len = ftell(f);
  fseek(f, 0, SEEK_SET);
  return len;
}

#include "sh_v2f.inc"
#include "MOD/sh_model_h.inc"

#include "ppmd2.hpp"

#include "utils.inc"

#include "transformer.inc"

static const uint CNUM = 256;

#include "newton.inc"

char cmap[CNUM];

int ppmd_order = 9;
int ppmd_memory = 6284; //1000;

// Searched in order when no weights file is given on the command line.
static const char* tf_weights_paths[] = {
  "6m-q4-fp32.tfwc2",
  "models/6m-q4-fp32.tfwc2",
  0
};

static const char* find_weights(const char* given) {
  const char* const one[] = { given, 0 };
  const char* const* list = given ? one : tf_weights_paths;
  for( int i = 0; list[i]; i++ ) {
    FILE* f = fopen(list[i], "rb");
    if( f ) {
      fclose(f);
      return list[i];
    }
  }
  return 0;   // also the "ppmd only" switch: name a file that does not exist
}

// The null device asks for a freshly initialized model rather than a file.
// It has to be recognized by name: on Windows fopen("nul") SUCCEEDS, so the
// loader would otherwise read an empty file and die, and on Linux
// "/dev/null" does the same.
static int is_null_device(const char* s) {
  static const char* names[] = { "nul", "nul:", "/dev/null", 0 };
  for( int i = 0; names[i]; i++ ) {
    int j = 0;
    for( ; s[j] && names[i][j]; j++ ) {
      int a = s[j], b = names[i][j];
      if( a>='A' && a<='Z' ) a += 'a'-'A';
      if( a!=b ) break;
    }
    if( !s[j] && !names[i][j] )
      return 1;
  }
  return 0;
}

ALIGN(64) Transformer tf;
ALIGN(64) Rangecoder rc;
ALIGN(64) UnifiedModel<Transformer> M;
ALIGN(64) BinaryMixer mixer[32];

int main(int argc, char** argv) {
  if( argc<4 ) {
    fprintf(stderr,
      "usage: coder0 c|d <input> <output> [weights_in] [weights_out]\n"
      "  weights_in   the model to start from; without it, 6m-q4-fp32.tfwc2\n"
      "               then models/6m-q4-fp32.tfwc2.  \"nul\" (or /dev/null)\n"
      "               starts from a freshly initialized model instead of a\n"
      "               file.  Naming a file that does not exist runs PPMD\n"
      "               alone.\n"
      "  weights_out  write the model back out when the file is done, so a\n"
      "               later run can start from it (needs TF_FP32=1; with\n"
      "               TF_TRAIN it is the trained model that gets written).\n");
    return 1;
  }
#if defined(__SSE3__)||defined(__x86_64__)||defined(_M_X64)
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

  uint f_DEC = (argv[1][0]=='d'), i, c, code, low, total;
  uint n_chars = 0, freq[CNUM], f_len, f_pos;

  FILE* f = fopen(argv[2], "rb");
  if( !f )
    return 2;
  FILE* g = fopen(argv[3], "wb");
  if( !g )
    return 3;

  for( i = 0; i<CNUM; i++ ) {
    cmap[i] = 0;
    freq[i] = 1;
  }

  if( f_DEC==0 ) {
    f_len = flen(f);
    fwrite(&f_len, 1, sizeof(f_len), g);
    for( f_pos = 0; f_pos<f_len; f_pos++ )
      cmap[getc(f)] = 1;
    fseek(f, 0, SEEK_SET);
    rc.StartEncode(g);
  } else {
    fread(&f_len, 1, sizeof(f_len), f);
    rc.StartDecode(f);
  }

  for( n_chars = 0, i = 0; i<CNUM; i++ )
    n_chars += (cmap[i] = rc.rc_BProcess(SCALE/2, cmap[i]));

  // The transformer replaces the LSTM.  If the weights are missing or the
  // alphabet does not fit the model's 205 tokens the model stays disabled and
  // its input to the mixer is PPMD's own distribution, which still round-trips
  // (just without the transformer's contribution).
  const char* warg = argc>4 ? argv[4] : 0;
#if !TF_LOAD_WEIGHTS
  warg = "nul";  // the build-time switch makes the same request as the name
#endif
  if( warg && is_null_device(warg) ) {
    // no file: the model is initialized in memory instead (weights_init.inc)
    fprintf(stderr, "coder0: transformer weights initialized from seed %llu\n",
            (unsigned long long)TF_INIT_SEED);
    if( !tf.Init(0, cmap, f_len) )
      fprintf(stderr, "coder0: transformer disabled "
                      "(alphabet does not fit the model)\n");
  } else {
    const char* wpath = find_weights(warg);
    if( !wpath || !tf.Init(wpath, cmap, f_len) )
      fprintf(stderr, "coder0: transformer disabled (%s)\n",
              wpath ? "alphabet does not fit the model" : "no weights file");
  }
#if TF_TRAIN>=3
  fprintf(stderr, "coder0: whole transformer trained online (%d params, "
                  "lr %g, batch %d)\n", 5897145,
          TF_FULL_LR_X1000000/1000000.0, (int)TF_BATCH);
#elif TF_TRAIN
  fprintf(stderr, "coder0: transformer output layer trained online\n");
#endif

  M.Init(ppmd_order, ppmd_memory, cmap, &tf, f_len);

  for( uint m_idx = 0; m_idx<32; ++m_idx )
    mixer[m_idx].Init();

  uint history = 0;

  for( f_pos = 0; f_pos<f_len; f_pos++ ) {
    uint ctx = history&31;
    mixer[ctx].Mix(M.tf_probs_, M.ppmd_probs_, cmap);

    // Final frequency calculation with safety bounds
    total = 0;
    float weight = 0.00f;
    for( i = 0; i<CNUM; i++ ) {
      if( cmap[i] ) {
        float p = (1.0f-weight)*mixer[ctx].probs_[i]+weight*M.ppmd_probs_[i];
        freq[i] = (uint)(p*SCALE);
        if( freq[i]<1 )
          freq[i] = 1;
      } else
        freq[i] = 0;
      total += freq[i];
    }

    // Renormalize to ensure sum <= SCALE
    if( total>SCALE ) {
      uint new_total = 0;
      for( i = 0; i<CNUM; i++ ) {
        if( cmap[i] ) {
          freq[i] = (uint)((qword)freq[i]*(SCALE-n_chars)/total)+1;
        }
        new_total += freq[i];
      }
      total = new_total;
    }

    if( f_DEC==0 ) {
      c = getc(f);
      for( low = 0, i = 0; i<c; i++ )
        low += freq[i];
      rc.rc_Process(low, freq[c], total);
    } else {
      code = rc.rc_GetFreq(total);
      for( c = 0, low = 0; low+freq[c]<=code; c++ )
        low += freq[c];
      rc.rc_Process(low, freq[c], total);
      putc(c, g);
    }

    uint bit = (c>' ');
    history = (history<<1)|bit;

    M.UpdatePPMD(c);
    M.UpdateTransformer(c);
    mixer[ctx].Update(c);
  }

  if( f_DEC==0 )
    rc.FinishEncode();
  fclose(g);
  fclose(f);

  // The model both sides end with is the same, so either can write it out;
  // saving from both and comparing the files is a check that they agree.
  if( argc>5 && tf.Ready() ) {
    if( tf.SaveWeights(argv[5]) )
      fprintf(stderr, "coder0: wrote the final model to %s\n", argv[5]);
    else
      return 4;
  }

  return 0;
}
