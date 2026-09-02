#define _FILE_OFFSET_BITS 64
// C library headers
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
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

ALIGN(64) Lstm lstm;
ALIGN(64) Rangecoder rc;
ALIGN(64) UnifiedModel<Lstm> M;
ALIGN(64) BinaryMixer mixer[32];

int main(int argc, char** argv) {
  if( argc<4 )
    return 1;
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

  srand(0xDEADBEEF);
  lstm.Init(n_chars, n_chars, lstm_num_cells, lstm_horizon, LSTM_LR_X100000/100000.0f, LSTM_CLIP_X10/10.0f, update_limit);
  M.Init(ppmd_order, ppmd_memory, cmap, &lstm, f_len);

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
    M.UpdateLSTM(c, M.ppmd_probs_);
    mixer[ctx].Update(c);
  }

  if( f_DEC==0 )
    rc.FinishEncode();
  fclose(g);
  fclose(f);
  return 0;
}
