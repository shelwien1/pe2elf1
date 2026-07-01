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
#include <pmmintrin.h>
#endif

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

#define _FILE_OFFSET_BITS 64


typedef uint16_t word;
typedef uint32_t uint;
typedef uint8_t byte;
typedef unsigned long long qword;

#include "ppmd.hpp"
//#include "ppmd3c1.hpp"
#include "sh_v2f.inc"

typedef float*__restrict prfloat;
typedef const float*__restrict cprfloat;

static inline float SumOfAbs(cprfloat a, uint len) {
  float sum = 0;
  for( uint i = 0; i<len; i++ )
    sum += fabs(a[i]);
  return sum;
}

struct UnifiedModel {
  ppmd_Model ppmd_model_;    // primary (high-order) model
  ppmd_Model ppmd_model2_;   // secondary (order-4) model
  char* vocab_;
  byte byte_map_[256];
  float ppmd_probs_[256];    // predictions of the primary model
  float ppmd_probs2_[256];   // predictions of the secondary model
  NOINLINE void Init(int order, int memory, int order2, int memory2, char* vocab) {
    vocab_ = vocab;
    ppmd_model_.Init(order, memory, 1, 0);
    ppmd_model2_.Init(order2, memory2, 1, 0);
    for( uint i = 0, offset = 0; i<256; i++ ) {
      byte_map_[i] = offset;
    }
    Renorm_probs(ppmd_probs_);
    Renorm_probs(ppmd_probs2_);
  }
  float packed_probs_[256];
  NOINLINE void Make_Packed() {
    for( uint i = 0, j = 0; i<256; i++ )
      if( vocab_[i] )
        packed_probs_[j++] = ppmd_probs_[i];
  }
  void Renorm_probs(prfloat probs) {
    float inv_sum = 1.0f/(SumOfAbs(probs, 256)+1e-12f);
    for( uint i = 0; i<256; ++i )
      probs[i] *= inv_sum;
  }
  NOINLINE void UpdatePPMD(uint byte_val) {
    ppmd_model_.ppmd_UpdateByte(byte_val&0xFF);
    ppmd_model_.ppmd_PrepareByte();
    for( uint i = 0; i<256; ++i )
      ppmd_probs_[i] = vocab_[i] ? (ppmd_model_.sqp[i]<1.0f ? 1.0f : ppmd_model_.sqp[i]) : 0.0f;
    Renorm_probs(ppmd_probs_);

    ppmd_model2_.ppmd_UpdateByte(byte_val&0xFF);
    ppmd_model2_.ppmd_PrepareByte();
    for( uint i = 0; i<256; ++i )
      ppmd_probs2_[i] = vocab_[i] ? (ppmd_model2_.sqp[i]<1.0f ? 1.0f : ppmd_model2_.sqp[i]) : 0.0f;
    Renorm_probs(ppmd_probs2_);
  }
};
static const uint CNUM = 256;
char cmap[CNUM];
int ppmd_order = 20;
int ppmd_memory = 1000;
int ppmd_order2 = 4;      // order of the secondary ppmd model
int ppmd_memory2 = 256;   // suballocator size (MB) for the secondary model
uint lstm_input_size = 128;
uint lstm_num_cells = 90;
uint lstm_horizon = 73;
uint LSTM_LEARNING_RATE_X100000 = 7200;
uint LSTM_GRADIENT_CLIP_X10 = 20;
uint update_limit = 3000;
ALIGN(64) Rangecoder rc;
ALIGN(64) UnifiedModel M;

// ---------------------------------------------------------------------------
// BinaryMixer support: definitions that binmix.inc expects the includer to
// provide. The mixer combines two probability distributions per bit-tree node
// using a logistic weight adapted online with a Newton-style update.
//
//   clip()  - symmetric clamp to [-lim, lim]
//   clamp() - clamp to [lo, hi]
//   st()    - stretch (logit): inverse of the sq() squash used by the mixer
//
// The F_* tuning constants are the float forms of the integer I_* tuning
// values documented at the top of binmix.inc, each scaled out of its
// fixed-point representation. These affect compression ratio only; encoder
// and decoder share them, so the stream stays losslessly reversible for any
// finite values. ADAPT_L=0 disables the optional loss-tracking accumulator.
// ---------------------------------------------------------------------------
#define ADAPT_L 0

static inline float clip( float x, float lim ) {
  return x < -lim ? -lim : (x > lim ? lim : x);
}
static inline float clamp( float x, float lo, float hi ) {
  return x < lo ? lo : (x > hi ? hi : x);
}
static inline float st( float p ) {
  p = clamp( p, 1.0f/65536, 1.0f-1.0f/65536 );
  return logf( p/(1.0f-p) );
}

static const float F_W0         = 13824.0f/65536.0f;   // initial mix weight (probability, ~0.21)
static const float F_Wclip      = 1024.0f/64.0f;       // |weight| clamp in logit domain (~16)
static const float F_Pmin       = 3072.0f/(1<<23);     // probability floor before log (~3.7e-4)
static const float F_LW         = 32764.0f/32768.0f;   // loss-tracking decay (unused; ADAPT_L=0)
static const float F_W_M1_F     = 192520.0f/262144.0f; // 1st-moment (gradient) decay (~0.73)
static const float F_W_M2_F     = 6144.0f/8192.0f;     // 2nd-moment (curvature) decay (~0.75)
static const float F_W_NW_F     = 261123.0f/262144.0f; // Newton step scale / learning rate (~1.0)
static const float F_W_INC_F    = 78532.0f/262144.0f;  // curvature epsilon in denominator (~0.30)
static const float F_W_STEP_F   = 51328.0f/4096.0f;    // max per-byte weight step (~12.5)
static const float F_W_G2_F     = 32.0f/16.0f;         // per-step curvature clip (~2.0)
static const float F_W_D_CLIP_F = 16000.0f/4096.0f;    // accumulated-gradient clip (~3.9)
static const float F_W_R_CLIP_F = 18692.0f/4096.0f;    // accumulated-curvature clip (~4.56)

#include "binmix.inc"

static const uint MIX_CTX = 32;
ALIGN(64) BinaryMixer mixer[MIX_CTX];

uint flen(FILE* f) {
  fseek(f, 0, SEEK_END);
  uint len = ftell(f);
  fseek(f, 0, SEEK_SET);
  return len;
}

int main(int argc, char** argv) {
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
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
    n_chars += (freq[i] = 1);
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
  M.Init(ppmd_order, ppmd_memory, ppmd_order2, ppmd_memory2, cmap);
  for( i = 0; i<MIX_CTX; i++ ) mixer[i].Init();
  uint history = 0;
  for( f_pos = 0; f_pos<f_len; f_pos++ ) {
    uint ctx = history & (MIX_CTX-1);
    // Mix the two ppmd models' predictions before (de)coding this byte.
    // The primary (high-order) model gets the larger initial weight.
    mixer[ctx].Mix( M.ppmd_probs_, M.ppmd_probs2_, cmap );
    for( total = 0, i = 0; i<CNUM; i++ ) {
      freq[i] = mixer[ctx].probs_[i]*SCALE;
      total += (freq[i] += ((freq[i]==0)&cmap[i]));
    }
    if( f_DEC==0 ) {
      for( low = 0, i = 0, c = getc(f); i<c; i++ )
        low += freq[i];
      rc.rc_Process(low, freq[c], total);
    } else {
      code = rc.rc_GetFreq(total);
      for( c = 0, low = 0; low+freq[c]<=code; c++ )
        low += freq[c];
      rc.rc_Process(low, freq[c], total);
    }
    if( f_DEC==1 )
      putc(c, g);
    mixer[ctx].Update(c);        // adapt the mixing weights on the coded byte
    M.UpdatePPMD(c);             // advance both ppmd models
    history = (history<<1) | (c>' ');
  }
  if( f_DEC==0 )
    rc.FinishEncode();
  fclose(g);
  fclose(f);
  return 0;
}
