// capfilt.cpp - model-driven, reversible case (bit5) filter (two passes).
//
// Derived from coder0.cpp. Instead of range-coding the input, it uses the two
// mixed PPMd models to predict, at each position, the conditional probability
// that the letter there is a capital, P(caps | letter) = capMass/(capMass +
// lowerMass) over the mixed next-symbol distribution. When that exceeds a
// threshold, bit5 (the ASCII case bit) of the current letter is flipped. No
// range coder is used and no side information is stored: the output is a
// byte-for-byte transform of the input.
//
// The filter runs TWO independent passes:
//   * pass 1 - left-to-right  (each decision uses LEFT  context)
//   * pass 2 - right-to-left  (each decision uses RIGHT context)
// The passes are fully independent: each starts from fresh models/mixers and
// nothing (no flip flags, no reversed buffers) is shared between them.
//
// Each pass is a self-inverting cap filter: it drives its model with the
// "canonical" symbol - the original byte on encode, the restored byte on
// decode - so the per-position probability, and hence the flip decision, is
// identical on both sides. Flipping bit5 of an English letter yields an
// English letter, so is_letter() matches on both sides too. Therefore a pass
// run in decode mode exactly undoes the same pass run in encode mode.
//
// Composing the two: encode applies pass 1 then pass 2; decode undoes them in
// the opposite order - pass 2 (right-to-left) first, then pass 1
// (left-to-right) - so decode(encode(x)) == x with no stored state.
//
//   encode ('c'):  buf --pass1 L->R--> --pass2 R->L-->  output
//   decode ('d'):  buf --pass2 R->L--> --pass1 L->R-->  output

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
    // Reset the prediction buffers so every pass starts from the same state
    // (they are reused across passes; the first byte would otherwise inherit
    // the previous pass's leftover probabilities and desync encode/decode).
    memset( ppmd_probs_, 0, sizeof(ppmd_probs_) );
    memset( ppmd_probs2_, 0, sizeof(ppmd_probs2_) );
    Renorm_probs(ppmd_probs_);
    Renorm_probs(ppmd_probs2_);
  }
  void Free() {
    // Release both suballocators so a following pass re-Init's from scratch
    // without doubling peak memory.
    ppmd_model_.StopSubAllocator();
    ppmd_model2_.StopSubAllocator();
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

// One cap-filter pass over buf[0..n-1].
//   dir  : +1 = left-to-right (left context), -1 = right-to-left (right context)
//   f_DEC: 0 = flip (encode), 1 = restore (decode)
// Fresh models/mixers each call, so passes are independent. Returns the flip
// count. The pass drives its models with the canonical (original) symbol - the
// input byte when encoding, the restored byte when decoding - so a decode-mode
// pass exactly undoes the matching encode-mode pass.
static qword cap_pass( byte* buf, qword n, int dir, uint f_DEC, float thr ) {
  M.Init(ppmd_order, ppmd_memory, ppmd_order2, ppmd_memory2, cmap);
  for( uint m = 0; m<MIX_CTX; m++ ) mixer[m].Init();

  uint history = 0;
  qword n_flip = 0;
  long long i = (dir>0) ? 0 : (long long)n-1;
  for( qword k = 0; k<n; k++, i += dir ) {
    uint in = buf[i];
    uint ctx = history & (MIX_CTX-1);

    // Mixed next-symbol prediction, then the conditional cap probability
    // P(caps | letter) = capMass / (capMass + lowerMass).
    mixer[ctx].Mix( M.ppmd_probs_, M.ppmd_probs2_, cmap );
    float capm = 0.0f, lowm = 0.0f;
    for( uint s = 'A'; s<='Z'; s++ ) capm += mixer[ctx].probs_[s];
    for( uint s = 'a'; s<='z'; s++ ) lowm += mixer[ctx].probs_[s];
    float denom = capm + lowm;
    float pcap = (denom>0.0f) ? capm/denom : 0.0f;

    // Flip the case bit of a letter when a capital is the likelier case. The
    // decision uses only already-processed (canonical) context and is_letter()
    // is invariant under the flip, so it matches on encode and decode.
    uint flip = (is_letter(in) && (pcap>thr)) ? 0x20 : 0;

    uint out, orig;
    if( f_DEC==0 ) {
      orig = in;            // encode sees the original byte...
      out  = in ^ flip;     // ...and emits the flipped version
    } else {
      out  = in ^ flip;     // decode restores the original...
      orig = out;           // ...which is what feeds the models
    }

    buf[i] = out;

    // Advance models/mixer with the canonical (original) symbol.
    mixer[ctx].Update(orig);
    M.UpdatePPMD(orig);
    history = (history<<1) | (orig>' ');
    n_flip += (flip!=0);
  }

  M.Free();
  return n_flip;
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

  // The two-pass filter needs the data in both directions, so load it whole.
  fseek(f, 0, SEEK_END);
  qword n = ftell(f);
  fseek(f, 0, SEEK_SET);
  byte* buf = (byte*)malloc( n ? n : 1 );
  if( !buf ) { fclose(f); fclose(g); return 4; }
  if( n && fread(buf, 1, n, f)!=n ) { fclose(f); fclose(g); free(buf); return 5; }
  fclose(f);

  // No range coder and no side information: the whole alphabet is available.
  for( uint i = 0; i<CNUM; i++ ) cmap[i] = 1;

  qword flips_LR, flips_RL;
  if( f_DEC==0 ) {
    // encode: pass 1 (left-to-right) then pass 2 (right-to-left)
    flips_LR = cap_pass( buf, n, +1, 0, cap_thr );
    flips_RL = cap_pass( buf, n, -1, 0, cap_thr );
  } else {
    // decode: undo pass 2 (right-to-left) first, then pass 1 (left-to-right)
    flips_RL = cap_pass( buf, n, -1, 1, cap_thr );
    flips_LR = cap_pass( buf, n, +1, 1, cap_thr );
  }

  if( n ) fwrite(buf, 1, n, g);
  fclose(g);
  free(buf);

  fprintf(stderr, "%s: %llu bytes, L->R flips=%llu, R->L flips=%llu (threshold %.4f)\n",
          f_DEC ? "decode" : "encode", n, flips_LR, flips_RL, cap_thr);
  return 0;
}
