#define _FILE_OFFSET_BITS 64
// Every system header the whole program needs, in one place: coder0 and the
// weights codec are one translation unit, and the codec's own sources
// (../tf/weights_*.inc) carry no #include <...> of their own.
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <vector>

// Intel SSE/AVX intrinsics to fix the denormal microcode penalty
#if defined(__SSE3__)||defined(__x86_64__)||defined(_M_X64)
#include <xmmintrin.h>
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

// ---------------------------------------------------------------------------
// Build-time switches
//
//   LSTM_TRAIN           1 (the default): the LSTM trains online, the way it
//                        always has.  0 freezes every weight - the recurrent
//                        state still evolves and the mixer still adapts, but
//                        no gradient step runs.  With a checkpoint loaded that
//                        is "use this model as it stands", and it is what
//                        makes a save/load round trip directly checkable:
//                        frozen, the model written at the end of a run is the
//                        one that was read at the start, byte for byte.
//
//   LSTM_SAVE_OPTIMIZER  1 (the default): weights_out also carries AdamW's
//                        second moments and step counters, so a later run
//                        continues the optimizer instead of restarting it.
//                        0 writes the weights alone, which is half the size.
//                        The loader takes either, and which one is better
//                        depends on what the checkpoint is for: continuing
//                        the same stream wants the optimizer state, moving to
//                        new data does better without it (see README.md).
//                        AdamW's FIRST moments are never written - beta1 is
//                        0.024, so a restored m is gone after two steps.
// ---------------------------------------------------------------------------
#ifndef LSTM_TRAIN
#define LSTM_TRAIN 1
#endif
#ifndef LSTM_SAVE_OPTIMIZER
#define LSTM_SAVE_OPTIMIZER 1
#endif

static const int MAX_CELLS = 256;
static const int MAX_INPUT = 256;
static const int MAX_OUTPUT = 256;
static const int MAX_HORIZON = 128;

struct ArrCells {
  float d[MAX_CELLS];
};
struct ArrInput {
  float d[MAX_INPUT];
};
struct ArrHidden {
  float d[MAX_CELLS+1];
};
struct ArrOutput {
  float d[MAX_OUTPUT];
};

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

#include "lstm_layer.inc"

#include "lstm.inc"

// The transformer version's weights format, shared rather than copied so the
// two coders cannot drift apart.  weights_write.inc uses the range-coder
// models weights_io_compressed.inc defines, so it has to come after it.
#include "../tf/weights_io.h"
#include "../tf/weights_io.inc"
#include "../tf/weights_io_compressed.inc"
#include "../tf/weights_write.inc"

#include "lstm_io.inc"

static const uint CNUM = 256;

#include "newton.inc"

char cmap[CNUM];

int ppmd_order = 9;
int ppmd_memory = 6284; //1000;
uint lstm_input_size = 128;
uint lstm_num_cells = 90;
uint lstm_horizon = 73;
uint LSTM_LR_X100000 = 7200;
uint LSTM_CLIP_X10 = 20;
uint update_limit = 3000;

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

ALIGN(64) Lstm lstm;
ALIGN(64) Rangecoder rc;
ALIGN(64) UnifiedModel<Lstm> M;
ALIGN(64) BinaryMixer mixer[32];

int main(int argc, char** argv) {
  if( argc<4 ) {
    fprintf(stderr,
      "usage: coder0 c|d <input> <output> [weights_in] [weights_out]\n"
      "  weights_in   the model to start from.  Without it, or with \"nul\"\n"
      "               (or /dev/null), the LSTM starts from its usual fresh\n"
      "               initialization.  Naming a file that does not exist runs\n"
      "               PPMD alone, as the transformer version does.\n"
      "  weights_out  write the model back out when the file is done, so a\n"
      "               later run can start from it.  The encoder and the\n"
      "               decoder end with the same model, so either can write it\n"
      "               and the two files must come out identical.  What it\n"
      "               knew about byte values this file does not use is\n"
      "               carried over from weights_in rather than dropped.\n"
      "\n"
      "Decoding needs the same weights_in the encoding used: like the order\n"
      "and memory settings, the model is not recorded in the archive.\n");
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

  // The weights go first.  Everything about a checkpoint except fitting it to
  // this file's alphabet can be settled before any output exists, and a run
  // that is going to fail should not leave a plausible-looking archive behind.
  const char* warg = argc>4 ? argv[4] : 0;
  int lstm_on = 1, have_weights = 0;
  fx2::WeightsFile wf;

  if( warg && !is_null_device(warg) ) {
    FILE* w = fopen(warg, "rb");
    if( w ) {
      fclose(w);
      if( !lstm_io::Open(wf, warg, lstm_num_cells, lstm_num_cells+1) )
        return 5;
      have_weights = 1;
    } else {
      // the transformer version's rule: naming a file that is not there asks
      // for the model to be left out
      fprintf(stderr, "coder0: LSTM disabled (no weights file %s)\n", warg);
      lstm_on = 0;
    }
  }

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

  srand(0xDEADBEEF);

  // Init() first even when a checkpoint is loaded: it is what gives every
  // column the checkpoint cannot speak for the value that belongs to THIS
  // file's alphabet.
  if( lstm_on ) {
    lstm.Init(n_chars, n_chars, lstm_num_cells, lstm_horizon, LSTM_LR_X100000/100000.0f, LSTM_CLIP_X10/10.0f, update_limit);
    if( have_weights && !lstm_io::Apply(lstm, wf, cmap, warg) ) {
      fclose(g);
      remove(argv[3]);
      return 5;
    }
  }
#if !LSTM_TRAIN
  fprintf(stderr, "coder0: LSTM frozen (no online training)\n");
#endif

  M.Init(ppmd_order, ppmd_memory, cmap, &lstm, f_len);

  // With the model out of the picture the mixer sees PPMD on both inputs,
  // which still round-trips - just without the LSTM's contribution.
  if( !lstm_on )
    memcpy(M.lstm_probs_, M.ppmd_probs_, sizeof(M.lstm_probs_));

  for( uint m_idx = 0; m_idx<32; ++m_idx )
    mixer[m_idx].Init();

  uint history = 0;

  for( f_pos = 0; f_pos<f_len; f_pos++ ) {
    uint ctx = history&31;
    mixer[ctx].Mix(M.lstm_probs_, M.ppmd_probs_, cmap);

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
    if( lstm_on )
      M.UpdateLSTM(c, M.ppmd_probs_);
    else
      memcpy(M.lstm_probs_, M.ppmd_probs_, sizeof(M.lstm_probs_));
    mixer[ctx].Update(c);
  }

  if( f_DEC==0 )
    rc.FinishEncode();
  fclose(g);
  fclose(f);

  // The model both sides end with is the same, so either can write it out;
  // saving from both and comparing the files is a check that they agree.
  if( argc>5 && lstm_on ) {
    if( lstm_io::Save(argv[5], lstm, cmap, LSTM_SAVE_OPTIMIZER, LSTM_TRAIN,
                      have_weights ? &wf : 0) )
      fprintf(stderr, "coder0: wrote the final model to %s\n", argv[5]);
    else
      return 4;
  }

  return 0;
}
