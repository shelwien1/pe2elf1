// dr3_model.inc - the context-mixing coder of dicrank3, shared by the tool
// (dicrank3.cpp) and its tuning harness (dr3_tune.cpp).
//
// Built from the components of the tsvcomp coder (copied into this folder):
// every binary decision goes through
//
//   NCX hashed Counter<CP_H0> cells       sh_counter.inc, knobs IDX/dr3-H0.idx
//   -> NGRP MixN<CP_X0,GN> group mixers   sh_mixN.inc, IDX/dr3-X0.idx
//   -> MixN<CP_X1,NGRP> final mixer       IDX/dr3-X1.idx
//   -> SSE<CP_S0>                         sh_SSE.inc, IDX/dr3-S0.idx
//   -> Mix2<CP_M1>( mixer, SSE )          sh_mix2.inc, IDX/dr3-M1.idx
//   -> binary range coder                 sh_v2f.inc
//
// and every stage learns end to end: the final error is chained back through
// the mixers into the counters (tsvcomp's A2 gradients).
//
// The contexts are what the decoder knows before each decision. There are
// four streams, in file order:
//
//   3 explicit-word set:  bitmap over frequency ranks (is the word explicit?)
//   2 membership bitmap:  the other candidates in BitmapKey order
//   1 explicit-word order: per word after the first, unary over the remaining
//                          explicit words sorted by 2 cos(prev) + cos(prev2)
//                          for the first UH positions, then an Elias-gamma escape
//   0 runs:               per tail word, most frequent first: "new run?", then
//                          a new run's index among the runs not started yet
//                          (bit tree), or unary over the started runs sorted by
//                          dicrank2's centroid score
//
// Contexts use word statistics from the counted text (length, document
// frequency, count, lowercase share, suffixes), candidate scores and their
// margins, and run statistics: size, recency, usual fit, and how many of a
// run's members share the word's suffix, case class, length class and count
// class.
//
// Floating point: the model uses float arithmetic. Encoder and decoder agree
// only if they evaluate every float operation the same way, so dicrank3 must
// be built without -ffast-math and without FMA contraction (both are checked
// below), with SSE2 floats (FLT_EVAL_METHOD 0), and runs with flush-to-zero
// and denormals-are-zero set (Dr3Fpu). All transcendental functions are
// Schraudolph's bit tricks (schrau.inc), not libm.

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <initializer_list>
#include <unordered_map>
#include <vector>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
#include <pmmintrin.h>
#include <xmmintrin.h>
#define DR3_X86 1
#endif

#ifdef __FAST_MATH__
#error "build dicrank3 without -ffast-math: encoder and decoder must round every float operation alike"
#endif
#if FLT_EVAL_METHOD != 0
#error "dicrank3 needs FLT_EVAL_METHOD 0 (SSE2 floats, not x87)"
#endif
#if defined(__clang__)
#pragma clang fp contract(off)
#elif defined(__GNUC__)
#pragma GCC optimize("fp-contract=off")
#endif

typedef unsigned short word;
typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned long long qword;

#define GRAD_HOOK(gd,gr,hc) ((void)0)
#define INLINE   inline __attribute__((always_inline))
#define NOINLINE __attribute__((noinline))
#define ALIGN(n) __attribute__((aligned(n)))

// ---- the components' prelude (as in tsvcomp.cpp), in the order their headers need it

#include "sh_v2f.inc"
static const int mSCALE = SCALE-1;
static const float iSCALE = 1.0f/SCALE;

#include "sh_mapping.inc"
#ifndef __min
#define __min(a,b) ((a)<(b)?(a):(b))
#define __max(a,b) ((a)>(b)?(a):(b))
#endif
#include "MOD/dr3-S0_p.inc"
#include "MOD/dr3-M1_p.inc"
#include "MOD/dr3-H0_p.inc"
#include "MOD/dr3-X0_p.inc"
#include "MOD/dr3-X1_p.inc"

static inline unsigned long long tbl_n( unsigned long long n ) { return n; }

static inline float clip(float x, float d) { return fminf(fmaxf(x, -d), d); }
static inline float clamp(float p) { return fminf(fmaxf(p, 1.0f), float(mSCALE)); }
static inline float clamp(float x, float min_val, float max_val) { return fminf(fmaxf(x, min_val), max_val); }

#include "schrau.inc"

static float rt_logf( float x ) { volatile float v = x; return logf(v); }
static float rt_expf( float x ) { volatile float v = x; return expf(v); }

#ifndef SSE_NB_MAX
#define SSE_NB_MAX 16
#endif
static constexpr int sse_nb_clamp( int nb ) { return nb<2 ? 2 : nb>SSE_NB_MAX ? SSE_NB_MAX : nb; }
struct SSE_Seeds { float a[SSE_NB_MAX], b[SSE_NB_MAX]; };
static SSE_Seeds sse_seeds( int nb, float lim, float K, float M, float mwP0, float T0 ) {
  SSE_Seeds s = {};
  for( int j=0; j<nb; j++ ) {
    float s_j = -lim + float(j)*(2.0f*lim/float(nb-1));
    float pm  = 1.0f/(1.0f+rt_expf( -(s_j / K) ));
    float q0  = (pm - mwP0*M) / (1.0f - M);
    q0 = clamp( q0, 1.0f/4096, 1.0f-1.0f/4096 );
    s.a[j] = q0*T0; s.b[j] = (1.0f-q0)*T0;
  }
  return s;
}

#define def_Config(Config) struct Config {\
  static const float momentum_D, momentum_R, NW, inc, stepMax, minVal,maxVal, grad2_clip, D_clip, R_clip, R0; \
  static const int NAG; };

#define CP_NAME     CP_S0
#define CP_PFX      S0_
#define CP_SSE      1
#include "config.hpp"

#define CP_NAME     CP_M1
#define CP_PFX      M1_
#include "config_mix2.hpp"

#define CP_NAME     CP_H0
#define CP_PFX      H0_
#include "config.hpp"

#define CP_NAME     CP_X0m
#define CP_PFX      X0_
#include "config_mix2.hpp"
#define CP_NAME     CP_X0
#define CP_BASE     CP_X0m
#define CP_PFX      X0_
#include "config_mixN.hpp"

#define CP_NAME     CP_X1m
#define CP_PFX      X1_
#include "config_mix2.hpp"
#define CP_NAME     CP_X1
#define CP_BASE     CP_X1m
#define CP_PFX      X1_
#include "config_mixN.hpp"

#include "sh_pupdater.inc"
#include "sh_counter.inc"
#include "sh_SSE.inc"
#include "sh_mix2.inc"
#include "sh_mixN.inc"

typedef Counter<CP_S0> SSE_Cell;
typedef SSE<CP_S0>     S0_SSE;
typedef Mix2<CP_M1>    Mix2b_Cell;

static const int NGRP = 7, GN = 4, NCX = NGRP * GN;   // 28 hashed models in 7 groups
typedef MixN<CP_X0,GN>   MixX0_Cell;
typedef MixN<CP_X1,NGRP> MixX1_Cell;

#include "MOD/dr3-S0_h.inc"
#include "MOD/dr3-M1_h.inc"
#include "MOD/dr3-H0_h.inc"
#include "MOD/dr3-X0_h.inc"
#include "MOD/dr3-X1_h.inc"

// ---- the model ------------------------------------------------------------

// One hashed context model: 2^TB Counter cells and a 32-bit check per slot. A
// slot whose check does not match is re-initialized and taken over.
struct HashCtr {
  std::vector<uint32_t>       chk;
  std::vector<Counter<CP_H0>> c;
  qword mask = 0;
  void Init( int bits ) {
    chk.assign( size_t(1) << bits, 0 );
    c.resize( size_t(1) << bits );
    for( auto& x : c ) x.Init();
    mask = (qword(1) << bits) - 1;
  }
  void Free() { std::vector<uint32_t>().swap( chk ); std::vector<Counter<CP_H0>>().swap( c ); }
  Counter<CP_H0>& Get( qword h ) {
    size_t i = size_t( h & mask );
    uint32_t k = uint32_t( h >> 32 ) | 1;
    if( chk[i] != k ) { chk[i] = k; c[i].Init(); }
    return c[i];
  }
};

// The streams, in file order
enum { kRuns = 0, kHead = 1, kBitmap = 2, kHeadSet = 3 };

struct Dr3Model {
  Rangecoder rc;
  S0_T S0;
  M1_T M1;
  H0_T H0;
  X0_T X0;
  X1_T X1;
  HashCtr ht[NCX];
  double bits[4] = {};       // ideal code length per stream
  qword  decisions[4] = {};
  qword  nocx[4] = {};       // ablation: contexts replaced by a constant (dr3_tune)
};
static Dr3Model* g_m;

static void ModelInit() {
  Dr3Model& m = *(g_m = new Dr3Model);
  m.H0.H0_Init();
  m.X0.X0_Init(); for( qword i=0; i<qword(X0_Cx_Volume); i++ ) m.X0.X0_tbl[i].Init();
  m.X1.X1_Init(); for( qword i=0; i<qword(X1_Cx_Volume); i++ ) m.X1.X1_tbl[i].Init();
  m.M1.M1_Init(); for( qword i=0; i<qword(M1_Cx_Volume); i++ ) m.M1.M1_tbl[i].Init();
  int tb = H0_TB < 12 ? 12 : H0_TB > 22 ? 22 : H0_TB;
  for( int i = 0; i < NCX; i++ ) m.ht[i].Init( tb );
  m.S0.S0_Init(); for( qword i=0; i<qword(S0_Cx_Volume); i++ ) S0_SSE::Init( &m.S0.S0_tbl[ i*CP_S0::NB ] );
}

static void ModelFree() {
  Dr3Model& m = *g_m;
  m.X0.X0_Quit(); m.X1.X1_Quit(); m.M1.M1_Quit(); m.S0.S0_Quit(); m.H0.H0_Quit();
  for( int i = 0; i < NCX; i++ ) m.ht[i].Free();
  delete g_m;
  g_m = nullptr;
}

// Floating-point environment of the coder: flush-to-zero and
// denormals-are-zero, restored on scope exit.
struct Dr3Fpu {
#ifdef DR3_X86
  unsigned int saved = _mm_getcsr();
  Dr3Fpu() { _MM_SET_FLUSH_ZERO_MODE( _MM_FLUSH_ZERO_ON ); _MM_SET_DENORMALS_ZERO_MODE( _MM_DENORMALS_ZERO_ON ); }
  ~Dr3Fpu() { _mm_setcsr( saved ); }
#endif
};

// One binary decision: its NCX context hashes and the selectors of the group
// mixers, the final mixer, the SSE row and the final Mix2.
struct BitCx { qword h[NCX]; uint sid, st, e; };

// Codes (or, when decoding, returns) one bit.
static int CodeBit( int bit, const BitCx& x ) {
  Dr3Model& m = *g_m;
  Counter<CP_H0>* hc[NCX]; Counter<CP_H0>::Pred hp[NCX];
  for( int i = 0; i < NCX; i++ ) {
    hc[i] = &m.ht[i].Get( (m.nocx[x.sid] >> i) & 1 ? qword(i + 1) * 0x9E3779B97F4A7C15ULL : x.h[i] );
    hp[i] = hc[i]->PredictF();
  }
  MixX0_Cell* gm[NGRP]; MixX0_Cell::Pred pg[NGRP];
  float zg[NGRP];
  for( int g = 0; g < NGRP; g++ ) {
    float zi[GN]; for( int j = 0; j < GN; j++ ) zi[j] = hp[g*GN + j].z;
    gm[g] = &m.X0.X0_tbl[ X0_MakeCx( g, x.sid, x.st, x.e ) ]; pg[g] = gm[g]->Mix( zi ); zg[g] = pg[g].z;
  }
  MixX1_Cell& fm = m.X1.X1_tbl[ X1_MakeCx( x.sid, x.st, x.e ) ];
  MixX1_Cell::Pred pfm = fm.Mix( zg );
  float zn = pfm.z;
  SSE_Cell* sr = &m.S0.S0_tbl[ qword( S0_MakeCx( x.sid, x.st, x.e ) ) * CP_S0::NB ];
  SSE_Pred<CP_S0> ps = S0_SSE::Predict( sr, zn );
  Mix2b_Cell& m1 = m.M1.M1_tbl[ M1_MakeCx( x.sid, x.st, x.e ) ];
  Mix2b_Cell::Pred pz = m1.Mix( zn, ps.z );
  float pf = sq( pz.z );                   // P(bit==0)
  uint p = uint( clamp( pf*float(SCALE) ) );
  bit = m.rc.rc_BProcess( p, bit );
  m.bits[x.sid] -= log2( (bit ? double(SCALE-p) : double(p)) / SCALE );
  m.decisions[x.sid]++;
  // end-to-end errors, chained back through the stages
  float e_f = pf - float(1 - bit);
  float e_s = e_f * pz.d2;                 // SSE output -> final
  float e_q = e_f * pz.d1 + e_s * ps.dzdz; // mixer output -> final, directly and through the SSE
  m1.Update( bit, pz, e_f );
  fm.Update( bit, pfm, e_q );
  for( int g = 0; g < NGRP; g++ ) {
    float e_g = e_q * pfm.d[g];
    gm[g]->Update( bit, pg[g], e_g );
    for( int j = 0; j < GN; j++ ) hc[g*GN + j]->C_Update( bit, hp[g*GN + j], 1.0f, e_g * pg[g].d[j] );
  }
  S0_SSE::Update( sr, bit, ps, e_s );
  return bit;
}

// ---- hashing and buckets ---------------------------------------------------

static inline qword Mix64( qword x ) {
  x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ULL; x ^= x >> 27; x *= 0x94D049BB133111EBULL; x ^= x >> 31;
  return x;
}
static qword HashV( std::initializer_list<long long> v ) {
  qword h = 0x243F6A8885A308D3ULL;
  for( long long x : v ) h = Mix64( h ^ qword(x) ) + 0x9E3779B97F4A7C15ULL;
  return h;
}
// Fills x.h from a table of NCX context tuples; seed separates the tables.
static void Hash( BitCx& x, int seed, const long long (*v)[4] ) {
  for( int i = 0; i < NCX; i++ ) x.h[i] = HashV({ seed + i, v[i][0], v[i][1], v[i][2], v[i][3] });
}
static inline int ILog2( qword m ) { int e = 0; while( m >>= 1 ) e++; return e; }
static inline int Cap( long long x, int hi ) { return x < 0 ? 0 : x > hi ? hi : int(x); }
// position bucket: 0..7 exact, then 2 per octave
static inline int PBucket( long long p ) {
  if( p < 8 ) return int(p);
  int e = ILog2( qword(p) );
  return Cap( 8 + 2*(e-3) + ((p >> (e-1)) & 1), 23 );
}
// share of a run's members with some property: 0 none, 7 all, else 1..6
static inline int Share( long long cnt, long long n ) { return cnt <= 0 || n <= 0 ? 0 : cnt >= n ? 7 : 1 + int(cnt*6/n); }

// ---- word features ----------------------------------------------------------

struct WordF {
  const char* w;       // spelling
  int len;
  uint count, df, low;
  uint rank;           // frequency rank (candidate order)
  int L, D, C, W;      // the BitmapKey fields: length/3, log2 df, log2 count, lowercase share 0..4
  int wf;              // lowercase share 0..15
  int rep;             // log2 of count per document
  qword s1, s2, s3;    // last 1, 2, 3 letters
  int f1, f2;          // first two letters
};

static WordF MakeWordF( const char* w, int len, uint count, uint df, uint low, uint rank ) {
  WordF f;
  f.w = w; f.len = len; f.count = count; f.df = df; f.low = low; f.rank = rank;
  f.L = std::min( len, 12 ) / 3;
  f.D = ILog2( qword(df) + 1 ); f.C = ILog2( qword(count) + 1 );
  f.W = count ? int( std::min<qword>( 4, 5ULL*low/count ) ) : 0;
  f.wf = count ? int( std::min<qword>( 15, 16ULL*low/count ) ) : 0;
  f.rep = Cap( ILog2( qword(count) + 1 ) - ILog2( qword(df) + 1 ), 7 );
  auto suf = [&]( int k ) { qword h = qword(k); for( int j = len > k ? len-k : 0; j < len; j++ ) h = h*32 + byte(w[j]-'a'+1); return h; };
  f.s1 = suf(1); f.s2 = suf(2); f.s3 = suf(3);
  f.f1 = len ? w[0] : 0;
  f.f2 = len > 1 ? w[1] : 0;
  return f;
}

// ---- an index coded as Elias gamma --------------------------------------------

// q in [0, m): the gamma code of q+1 - the exponent in unary, then the bits
// below the leading 1, MSB first (a bit that would leave [1, m] is not coded).
// a, b, c: small context values of the caller.
static uint CodeGamma( int dec, uint q, uint m, uint sid, uint st, int seed, long long a, long long b, long long c ) {
  int emax = ILog2( m );
  int e = dec ? 0 : ILog2( qword(q) + 1 );
  int k = 0;
  for( ; k < emax; k++ ) {
    BitCx x; x.sid = sid; x.st = st; x.e = Cap( k, 7 );
    const long long v[NCX][4] = {
      { k }, { k, a }, { k, b }, { k, c },
      { k, a, b }, { k, a, c }, { k, b, c }, { k, emax },
      { k, a, b, c }, { k, emax, a }, { k, emax, b }, { k, emax, c },
      { 0 }, { k, a >> 1 }, { k, b >> 1 }, { k, c >> 1 },
      { k, a >> 1, b >> 1 }, { k, a >> 1, c >> 1 }, { k, b >> 1, c >> 1 }, { k, a, emax, c },
      { 1 }, { 2 }, { 3 }, { 4 }, { 5 }, { 6 }, { 7 }, { 8 },
    };
    Hash( x, seed, v );
    if( !CodeBit( dec ? 0 : e > k, x ) ) break;
  }
  e = k;
  uint t = 1, v1 = q + 1;
  for( int bi = e - 1; bi >= 0; bi-- ) {
    if( qword(t*2 + 1) << bi > m ) { t = t*2; continue; }
    BitCx x; x.sid = sid; x.st = st; x.e = 8 + Cap( e, 7 );
    long long tp = t & ((1u << (e - 1 - bi)) - 1);   // the bits below the leading 1 so far
    const long long v[NCX][4] = {
      { e, bi, tp }, { e, bi }, { e, bi, tp, a }, { e, bi, tp, b },
      { e, bi, tp, c }, { e, bi, a }, { e, bi, b }, { e, bi, c },
      { bi, tp >> 1 }, { e, bi, tp >> 2 }, { e, bi, a, b }, { 0 },
      { bi }, { bi, a }, { bi, b }, { bi, c },
      { 1 }, { 2 }, { 3 }, { 4 }, { 5 }, { 6 }, { 7 }, { 8 },
      { 9 }, { 10 }, { 11 }, { 12 },
    };
    Hash( x, seed + 50, v );
    t = t*2 + CodeBit( dec ? 0 : int((v1 >> bi) & 1), x );
  }
  return t - 1;
}

// ---- streams 3 and 2: bitmaps ----------------------------------------------------

// Codes bits[i] of the words w[i] (sid kHeadSet or kBitmap).
static void CodeBitmap( int dec, int sid, size_t n, const WordF* const* w, uint8_t* bits ) {
  qword h = 0;                                            // bit history
  std::unordered_map<qword, std::pair<uint,uint>> grp;   // BitmapKey group -> (ones, bits)
  int runlen = 0, lastbit = 0;
  for( size_t i = 0; i < n; i++ ) {
    const WordF& f = *w[i];
    qword key = ((qword(f.L) * 32 + f.D) * 32 + f.C) * 8 + f.W;
    auto& g = grp[key];
    int dens = g.second ? Cap( 1 + g.first * 14 / g.second, 15 ) : 0;
    int dn = Cap( ILog2( g.second + 1 ), 7 );
    int rl = Cap( ILog2( runlen + 1 ), 7 ) * 2 + lastbit;
    int rq = ILog2( qword(f.rank) + 1 );
    long long k = (long long)key, h1 = h & 1, h2 = h & 3, h4 = h & 15, h8 = h & 255;
    BitCx x; x.sid = sid; x.st = 0; x.e = Cap( f.W * 3 + (dens > 7) + (dens > 11), 15 );
    const long long v[NCX][4] = {
      { 0 }, { k }, { k, h4 }, { h8 },
      { (long long)(h & 0xFFFF) }, { k, dens, dn }, { (long long)f.s2 }, { (long long)f.s3 },
      { f.len, f.wf }, { f.rep, f.W, f.L }, { f.C, f.D, f.W }, { f.f1, f.L },
      { f.wf, (long long)f.s1 }, { h2, dens }, { rq, f.W }, { rl, k },
      { (long long)f.s3, f.W }, { f.D, f.rep, h1 }, { k, h1, dens }, { (long long)f.s1, f.L, f.W },
      { rl, dens }, { (long long)(h & 0xFFFFFF) }, { Cap( f.count, 255 ), f.W }, { f.f1, f.f2 },
      { k, h8 }, { f.C, f.rep, f.W, f.L }, { (long long)f.s2, f.L, f.W }, { (long long)(h & 0xFFFFFFFF) },
    };
    Hash( x, 6000 + 100*sid, v );
    int b = CodeBit( dec ? 0 : bits[i], x );
    if( dec ) bits[i] = uint8_t(b);
    g.first += b; g.second++;
    h = h*2 + b;
    if( b == lastbit ) runlen++; else { runlen = 1; lastbit = b; }
  }
}

// ---- stream 1: explicit-word order ------------------------------------------------

static const int UH = 16;   // unary positions before the gamma escape

static int CommonPrefix( const WordF& a, const WordF& b ) {
  int i = 0;
  while( i < a.len && i < b.len && a.w[i] == b.w[i] ) i++;
  return i;
}

// seq[0..n): the explicit words (indices into w and gram) in dictionary order;
// gram: their n x n similarity matrix (cosine x ~2^30).
static void CodeHead( int dec, uint n, const WordF* const* w, const int32_t* gram, uint32_t* seq ) {
  if( !n ) return;
  std::vector<char> used( n, 0 );
  std::vector<uint> rem;
  std::vector<int64_t> score( n );
  {  // the first word: a bit tree
    uint t = 0, first = dec ? 0 : seq[0];
    for( int b = n > 1 ? ILog2( n - 1 ) : -1; b >= 0; b-- ) {
      BitCx x; x.sid = kHead; x.st = 2; x.e = 0;
      for( int i = 0; i < NCX; i++ ) x.h[i] = HashV({ 4000 + i, b, i < 4 ? (long long)t : 0 });
      t = t*2 + CodeBit( dec ? 0 : int((first >> b) & 1), x );
    }
    if( t >= n ) { fprintf( stderr, "dicrank3: bad first explicit word\n" ); exit( 1 ); }
    seq[0] = t; used[t] = 1;
  }
  int ph1 = 0, ph2 = 0;   // position buckets of the last two words
  for( uint i = 1; i < n; i++ ) {
    uint p1 = seq[i-1];
    long long p2 = i >= 2 ? (long long)seq[i-2] : -1, p3 = i >= 3 ? (long long)seq[i-3] : -1;
    rem.clear();
    for( uint c = 0; c < n; c++ ) {
      if( used[c] ) continue;
      rem.push_back( c );
      score[c] = 2 * (int64_t)gram[size_t(p1)*n + c] + ( p2 >= 0 ? gram[size_t(p2)*n + c] : 0 );
    }
    std::sort( rem.begin(), rem.end(), [&]( uint a, uint b ) { return score[a] != score[b] ? score[a] > score[b] : a < b; } );
    uint truth = dec ? 0 : seq[i];
    const WordF& w1 = *w[p1];
    int tier = i < 80 ? 0 : 1, iq = ILog2( i );
    int top = Cap( score[rem[0]] >> 27, 31 );
    size_t nr = rem.size(), p = 0, ul = std::min<size_t>( UH, nr - 1 );
    int found = 0;
    for( ; p < ul; p++ ) {
      uint c = rem[p];
      const WordF& wc = *w[c];
      int64_t s = score[c];
      int pb = PBucket( p );
      int sq_ = Cap( s >> 27, 31 );
      int mq = Cap( (score[rem[0]] - s) >> 26, 63 );
      int gq = Cap( (s - score[rem[p+1]]) >> 23, 63 );
      int c1 = Cap( gram[size_t(p1)*n + c] >> 26, 16 );
      int c2 = p2 >= 0 ? Cap( gram[size_t(p2)*n + c] >> 26, 16 ) : 17;
      int c3 = p3 >= 0 ? Cap( gram[size_t(p3)*n + c] >> 26, 16 ) : 17;
      int rmax = 0;
      for( uint k = 1; k <= 8 && k <= i; k++ ) rmax = std::max( rmax, Cap( gram[size_t(seq[i-k])*n + c] >> 26, 16 ) );
      int fr = ILog2( c + 1 );
      long long dd = (long long)c - p1;
      int fd = (dd < 0) * 16 + ILog2( qword( dd < 0 ? -dd : dd ) + 1 );
      int cp = std::min( CommonPrefix( wc, w1 ), 7 );
      int morph = cp >= 3 ? cp * 4 + Cap( (long long)wc.len - w1.len + 1, 3 ) : cp;
      int p7 = pb > 7 ? 7 : pb;
      BitCx x; x.sid = kHead; x.st = tier; x.e = Cap( pb, 15 );
      const long long v[NCX][4] = {
        { pb }, { pb, sq_ }, { pb, mq }, { sq_, mq },
        { pb, gq }, { c1, c2 }, { pb, c3 }, { c1, c3 },
        { pb, tier }, { pb, iq }, { pb, fr }, { fd, p7 },
        { morph, p7 }, { morph, c1 }, { rmax, pb }, { rmax, c1 },
        { (long long)wc.s2, (long long)w1.s2 }, { wc.L, w1.L, p7 }, { pb, sq_, c3 }, { mq, gq },
        { 0 }, { pb, top }, { wc.W, w1.W }, { wc.C - w1.C, p7 },
        { pb, ph1 }, { pb, ph1, ph2 }, { sq_, ph1 }, { mq >> 1, ph1, tier },
      };
      Hash( x, 5000, v );
      if( CodeBit( dec ? 0 : c == truth, x ) ) { found = 1; break; }
    }
    if( !found ) {
      if( nr <= UH ) p = nr - 1;
      else {
        uint q = 0;
        if( !dec ) q = uint( std::find( rem.begin(), rem.end(), truth ) - rem.begin() ) - UH;
        int mU = Cap( (score[rem[0]] - score[rem[UH]]) >> 26, 63 );
        p = UH + CodeGamma( dec, q, uint( nr - UH ), kHead, 3, 8000, ph1, tier * 16 + iq, mU >> 2 );
      }
    }
    if( p >= nr || (!dec && rem[p] != truth) ) { fprintf( stderr, "dicrank3: bad explicit-word position\n" ); exit( 1 ); }
    seq[i] = rem[p]; used[rem[p]] = 1;
    ph2 = ph1; ph1 = PBucket( p );
  }
}

// ---- stream 0: runs --------------------------------------------------------------

// run[j]: the run of tail word j (words most frequent first), R runs, numbered
// in dictionary order. The provider gives each word's score for every run
// (Score(j), 0 for runs not started) and learns the word's run (Add(j, r)),
// in that order - dicrank2's centroids in dicrank3, a dump in dr3_tune.
template<class Provider>
static void CodeRuns( int dec, uint T, uint R, const WordF* const* w, uint32_t* run, Provider& prov ) {
  std::vector<char> started( R, 0 );
  std::vector<uint> nmem( R, 0 ), last( R, 0 ), sumW( R, 0 ), sumC( R, 0 ), sumFit( R, 0 );
  // members of each run by case class W (0..4), length class L (0..4) and count class C (0..31)
  std::vector<uint> byW( size_t(R) * 5, 0 ), byL( size_t(R) * 5, 0 ), byC( size_t(R) * 32, 0 );
  std::unordered_map<qword, uint> suf;   // (run, suffix) -> members with that suffix
  auto sufn = [&]( uint r, qword s ) { auto it = suf.find( (qword(r) << 40) ^ s ); return it == suf.end() ? 0u : it->second; };
  std::vector<uint> unstarted( R ); for( uint r = 0; r < R; r++ ) unstarted[r] = r;
  std::vector<uint> order; order.reserve( R );
  uint nstarted = 0, prun = ~0u;
  int ppos = 0, ppos2 = 0;   // position buckets of the last two words (24 = new run)
  for( uint j = 0; j < T; j++ ) {
    const WordF& f = *w[j];
    const int64_t* sc = prov.Score( j );
    order.clear();
    for( uint r = 0; r < R; r++ ) if( started[r] ) order.push_back( r );
    std::sort( order.begin(), order.end(), [&]( uint a, uint b ) { return sc[a] != sc[b] ? sc[a] > sc[b] : a < b; } );
    uint truth = dec ? 0 : run[j];
    int jq = ILog2( j + 1 );
    int nuq = ILog2( R - nstarted + 1 );
    int top = order.empty() ? 0 : Cap( sc[order[0]] >> 30, 40 );
    int ta3 = order.empty() ? 0 : Share( sufn( order[0], f.s3 ), nmem[order[0]] );
    int ta2 = order.empty() ? 0 : Share( sufn( order[0], f.s2 ), nmem[order[0]] );
    int gap01 = order.size() < 2 ? 63 : Cap( (sc[order[0]] - sc[order[1]]) >> 27, 63 );
    // 1. a new run?
    int isnew;
    if( order.empty() ) isnew = 1;
    else if( nstarted == R ) isnew = 0;
    else {
      BitCx x; x.sid = kRuns; x.st = 0; x.e = Cap( nuq, 15 );
      const long long v[NCX][4] = {
        { 0 }, { nuq }, { jq }, { nuq, jq },
        { top }, { top, nuq }, { f.W }, { f.C },
        { f.W, top }, { ta3 }, { ta3, top }, { ta2, top },
        { (long long)f.s2 }, { (long long)f.s3 }, { f.rep, f.W }, { f.L, f.W },
        { top, jq }, { ta2, ta3 }, { f.D }, { f.C, f.D },
        { nuq, f.W }, { top >> 2, ta3, f.W }, { jq >> 1, top >> 1 }, { ppos },
        { ppos, ppos2 }, { gap01 >> 2, top >> 2 }, { gap01 >> 1 }, { top >> 1, f.wf },
      };
      Hash( x, 1000, v );
      isnew = CodeBit( dec ? 0 : !started[truth], x );
    }
    uint r;
    int pos_b;
    if( isnew ) {
      // 2. which of the runs not started yet: its index k, a bit tree
      uint nu = R - nstarted, k = 0;
      if( !dec ) k = uint( std::find( unstarted.begin(), unstarted.end(), truth ) - unstarted.begin() );
      uint t = 0;
      for( int b = nu > 1 ? ILog2( nu - 1 ) : -1; b >= 0; b-- ) {
        if( ((t*2 + 1) << b) > nu - 1 ) { t = t*2; continue; }   // a 1 would leave the range
        BitCx x; x.sid = kRuns; x.st = 2; x.e = Cap( b, 15 );
        const long long v[NCX][4] = {
          { b, t }, { b }, { b, t, nuq }, { b, t, jq },
          { b, t >> 1 }, { b, t >> 2 }, { b, nuq }, { b, jq },
          { b, t, f.W }, { b, t, f.C }, { b, top }, { 0 },
          { b, t, (long long)f.s2 }, { b, t >> 1, f.W }, { b, nu }, { b, t, nu },
          { 1 }, { 2 }, { 3 }, { 4 }, { 5 }, { 6 }, { 7 }, { 8 },
          { 9 }, { 10 }, { 11 }, { 12 },
        };
        Hash( x, 2000, v );
        t = t*2 + CodeBit( dec ? 0 : int((k >> b) & 1), x );
      }
      if( t >= nu ) { fprintf( stderr, "dicrank3: bad new-run index\n" ); exit( 1 ); }
      r = unstarted[t];
      unstarted.erase( unstarted.begin() + t );
      started[r] = 1; nstarted++;
      pos_b = 24;
    } else {
      // 3. unary over the started runs by score
      size_t nr = order.size(), p = 0;
      for( ; p + 1 < nr; p++ ) {
        uint c = order[p];
        int64_t s = sc[c];
        int pb = PBucket( p );
        int cq = Cap( s >> 30, 40 );                            // cosine x 32
        int mq = Cap( (sc[order[0]] - s) >> 29, 63 );           // margin to the top, cosine x 64
        int gq = Cap( (s - sc[order[p+1]]) >> 27, 63 );         // gap to the next, cosine x 256
        int rn = ILog2( nmem[c] + 1 );
        int rc_ = ILog2( j - last[c] + 1 );
        int a1 = Share( sufn( c, f.s1 ), nmem[c] ), a2 = Share( sufn( c, f.s2 ), nmem[c] ), a3 = Share( sufn( c, f.s3 ), nmem[c] );
        int lr = nmem[c] ? int( sumW[c] / nmem[c] ) : 0;
        int cr = nmem[c] ? int( sumC[c] / nmem[c] ) : 0;
        int fit = nmem[c] ? Cap( 20 + cq - int( sumFit[c] / nmem[c] ), 40 ) : 0;   // score vs the run's usual fit
        int same = c == prun;
        int p3 = pb > 3 ? 3 : pb;
        int aW = Share( byW[ size_t(c) * 5 + f.W ], nmem[c] ), aL = Share( byL[ size_t(c) * 5 + f.L ], nmem[c] ),
            aC = Share( byC[ size_t(c) * 32 + std::min( f.C, 31 ) ], nmem[c] );
        BitCx x; x.sid = kRuns; x.st = 1; x.e = Cap( pb, 15 );
        const long long v[NCX][4] = {
          { pb }, { pb, cq }, { pb, aW }, { cq, mq },
          { pb, gq }, { aW, cq }, { aW, aL, aC }, { cq, rn },
          { pb, rc_ }, { f.wf, c }, { cq, a3 }, { a1, a2, a3 },
          { f.wf >> 1, lr, pb }, { f.C, cr, p3 }, { c }, { c, p3 },
          { (long long)f.s2, c }, { (long long)f.s3, c }, { f.W, c }, { f.L, c },
          { f.rep, c }, { mq, gq }, { f.C, c }, { aW, a2, p3 },
          { pb, fit }, { fit, rn }, { pb, same, ppos }, { c, fit >> 2 },
        };
        Hash( x, 3000, v );
        if( CodeBit( dec ? 0 : c == truth, x ) ) break;
      }
      if( p >= nr ) { fprintf( stderr, "dicrank3: bad run position\n" ); exit( 1 ); }
      r = order[p];
      pos_b = PBucket( p );
    }
    if( !dec && r != truth ) { fprintf( stderr, "dicrank3: run coding mismatch\n" ); exit( 1 ); }
    run[j] = r;
    prov.Add( j, r );
    // the word joins its run
    sumFit[r] += nmem[r] ? Cap( sc[r] >> 30, 40 ) : 0;
    nmem[r]++; last[r] = j; sumW[r] += f.wf >> 1; sumC[r] += f.C;
    byW[ size_t(r) * 5 + f.W ]++; byL[ size_t(r) * 5 + f.L ]++; byC[ size_t(r) * 32 + std::min( f.C, 31 ) ]++;
    suf[ (qword(r) << 40) ^ f.s1 ]++; suf[ (qword(r) << 40) ^ f.s2 ]++; suf[ (qword(r) << 40) ^ f.s3 ]++;
    prun = r; ppos2 = ppos; ppos = pos_b;
  }
}
