// capfilt.cpp - model-driven, reversible case (bit5) filter.
//
// Derived from coder0.cpp. Instead of range-coding the input, it uses the two
// mixed PPMd models to predict, at each position, the probability that the
// next symbol is a capital English letter 'A'..'Z'. When that probability
// exceeds a threshold, bit5 (the ASCII case bit) of the current letter is
// flipped in the output. No range coder is used and no side information is
// stored: the output is a byte-for-byte transform of the input.
//
// The transform is its own inverse given the shared model:
//   * encode ('c'): the input holds the ORIGINAL bytes; the models are
//     updated with those originals and the flipped bytes are emitted.
//   * decode ('d'): the input holds the transformed bytes; each letter is
//     un-flipped to restore the original, and the models are updated with the
//     restored (original) symbols.
// Because both directions drive the models with identical (original) symbols,
// the per-position cap probability - and hence the flip decision - is the same
// on each side, so decode exactly reverses encode. Flipping bit5 of an English
// letter always yields an English letter, so "is this byte a letter" is
// invariant under the flip and matches on both sides too.

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
// No sh_v2f.inc: the range coder is not needed for a byte-for-byte transform.

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
ALIGN(64) UnifiedModel M;

// ---------------------------------------------------------------------------
// BinaryMixer support: definitions that binmix.inc expects the includer to
// provide (see coder0.cpp for the full commentary). clip()/clamp()/st() plus
// the F_* tuning constants below are the hooks the mixer relies on; the values
// only affect how good the predictions are, never correctness of the round
// trip (encoder and decoder share them). ADAPT_L=0 drops the optional loss
// accumulator.
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

static inline int is_letter( uint c ) {
  return (c>='A' && c<='Z') || (c>='a' && c<='z');
}

int main(int argc, char** argv) {
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

  if( argc<4 ) {
    fprintf(stderr, "usage: %s c|d <input> <output> [cap_threshold=0.5]\n", argv[0]);
    return 1;
  }
  uint f_DEC = (argv[1][0]=='d');           // 0 = encode (flip), 1 = decode (restore)
  float cap_thr = (argc>4) ? (float)atof(argv[4]) : 0.5f;

  FILE* f = fopen(argv[2], "rb");
  if( !f )
    return 2;
  FILE* g = fopen(argv[3], "wb");
  if( !g )
    return 3;

  // No range coder and no side information: the whole alphabet is available.
  for( uint i = 0; i<CNUM; i++ ) cmap[i] = 1;

  M.Init(ppmd_order, ppmd_memory, ppmd_order2, ppmd_memory2, cmap);
  for( uint i = 0; i<MIX_CTX; i++ ) mixer[i].Init();

  uint history = 0;
  qword n_bytes = 0, n_flip = 0;
  int ch;
  while( (ch = getc(f)) != EOF ) {
    uint in = ch & 0xFF;
    uint ctx = history & (MIX_CTX-1);

    // Mixed prediction of the next symbol, then P(symbol in 'A'..'Z').
    mixer[ctx].Mix( M.ppmd_probs_, M.ppmd_probs2_, cmap );
    float pcap = 0.0f;
    for( uint s = 'A'; s<='Z'; s++ ) pcap += mixer[ctx].probs_[s];

    // Flip the case bit of a letter when a capital is predicted likely. The
    // decision depends only on past (original) context, so it matches on both
    // sides; flipping a letter keeps it a letter, so is_letter() matches too.
    uint flip = (is_letter(in) && (pcap>cap_thr)) ? 0x20 : 0;

    uint out, orig;
    if( f_DEC==0 ) {
      orig = in;            // encoder sees the original byte...
      out  = in ^ flip;     // ...and emits the flipped version
    } else {
      out  = in ^ flip;     // decoder restores the original...
      orig = out;           // ...which is what feeds the model
    }

    putc(out, g);

    // Both directions advance the models with the ORIGINAL symbol.
    mixer[ctx].Update(orig);
    M.UpdatePPMD(orig);
    history = (history<<1) | (orig>' ');

    n_bytes++;
    n_flip += (flip!=0);
  }

  fclose(g);
  fclose(f);

  fprintf(stderr, "%s: %llu bytes, %llu case-flips (threshold %.4f)\n",
          f_DEC ? "decode" : "encode",
          (qword)n_bytes, (qword)n_flip, cap_thr);
  return 0;
}
