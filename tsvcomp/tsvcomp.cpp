// tsvcomp.cpp -- compressor for the target columns t0, t1 of a TSV
// (connectome/quantize output), using every other column as context:
// the predictions pred_t0/pred_t1, the features, need_prediction.  Built from
// coder0's model: adaptive counter tables mixed by 2-input mixers and an SSE
// stage (../sh_counter.inc, ../sh_mix2.inc, ../sh_SSE.inc), with tsvcomp's
// own contexts (IDX/tc_model-*.idx).
//
//   tsvcomp c  input.tsv output.tsv output.rc   t0, t1 -> output.rc (compressed),
//                                               the other columns -> output.tsv
//   tsvcomp c  input.tsv output.rc              the same without writing output.tsv
//                                               (the form IDX/opt.pl runs: "exe c in out")
//   tsvcomp d  output.tsv output.rc restored.tsv
//   tsvcomp tc input.tsv output.tsv cols.tsv    t0, t1 as text, no header
//   tsvcomp td output.tsv cols.tsv restored.tsv
//   tsvcomp bc input.tsv output.tsv cols.bin    t0, t1 as int32 LE pairs, no header
//   tsvcomp bd output.tsv cols.bin restored.tsv
//
// The target columns are the ones named t0 and t1 (anything after a '/' in
// the header is the quantize scale: "t0/10000").  restored.tsv is
// byte-identical to input.tsv; the targets must be canonical int32 decimals.
// The .rc stream stores the columns' names and positions; cols.tsv/cols.bin
// keep neither: td/bd append them as the last two columns (where quantize
// puts them), named t0/t1 with the scale suffix of output.tsv's first scaled
// column.
//
// .rc format: "TSC2", u32 row count (without the header), then for t0 and
// t1: u16 column position, u8 name length + name; then the range coder stream.
//
// Binarization of a target v of column k (k = 0, 1; row by row, t0 before t1):
//   d = v - (previous row's value of column k)
//   zero flag [d == 0]; sign [d < 0]; e = floor(log2 |d|) in unary
//   ([e > j] for j = 0, 1, ..); the e bits of |d| below its leading 1, MSB
//   first, each in the context of the bits above it (a bit tree: recurring
//   jump sizes are learned exactly).
// Every bit: p_n = mix2( mix2( C0, C1 ), C2 ), p = mix2( p_n, SSE(p_n) )
// (FINAL_MIX 0: p = SSE(p_n)); the end-to-end gradients are chained back
// through every path.  Contexts (IDX/tc_model-*.idx):
//   C0  hash(need_prediction, k, node, e of the previous delta)
//       x rows since column k last changed x side-info change flags
//       (i0 price-like / volume-like / trade columns changed since the last row)
//   C1  hash(k, node, column k's last nonzero delta, the other column's
//       delta: this row's for t1, the previous row's for t0)
//   C2  hash(k, node, pred_tk and the current target level in linear buckets
//       of 1024 (clipped to +-15), change of pred_tk since the last row as a
//       sign + exponent bucket)
//   M0, M1  k x stage (zero/sign/exponent/mantissa) x exponent (step)
//   M2  k x stage x the pred_tk - level bucket
//   S0  k x stage x rows since the last change
// Without pred_t0/pred_t1 columns the prediction contexts see 0.

#include <stdlib.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdarg>
#include <string>
#include <vector>

#if defined(__SSE3__) || defined(__x86_64__) || defined(_M_X64)
#include <x86intrin.h>
#endif

typedef unsigned short word;
typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned long long qword;

#define GRAD_HOOK(gd,gr,hc) ((void)0)

#ifdef __GNUC__
 #define INLINE   __attribute__((always_inline))
 #define NOINLINE __attribute__((noinline))
 #define ALIGN(n) __attribute__((aligned(n)))
 #define restrict __restrict
#else
 #define INLINE   __forceinline
 #define NOINLINE __declspec(noinline)
 #define ALIGN(n) __declspec(align(n))
#endif

// ---- coder0's prelude (coder0.cpp), in the order its headers need it ----

#include "../sh_v2f.inc"
static const int mSCALE = SCALE-1;
static const float iSCALE = 1.0f/SCALE;

#include "../sh_mapping.inc"
// the threshold mappings idx2inc.pl expands into lookup tables use these
#ifndef __min
#define __min(a,b) ((a)<(b)?(a):(b))
#define __max(a,b) ((a)>(b)?(a):(b))
#endif
#include "MOD/tc_model-C0_p.inc"
#include "MOD/tc_model-C1_p.inc"
#include "MOD/tc_model-C2_p.inc"
#include "MOD/tc_model-S0_p.inc"
#include "MOD/tc_model-M0_p.inc"
#include "MOD/tc_model-M1_p.inc"
#include "MOD/tc_model-M2_p.inc"

static inline unsigned long long tbl_n( unsigned long long n ) { return n; }

static inline float clip(float x, float d) { return fminf(fmaxf(x, -d), d); }
static inline float clamp(float p) { return fminf(fmaxf(p, 1.0f), float(mSCALE)); }
static inline float clamp(float x, float min_val, float max_val) { return fminf(fmaxf(x, min_val), max_val); }

#include "../schrau.inc"

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

#define CP_NAME     CP_C0
#define CP_PFX      C0_
#include "../config.hpp"

#define CP_NAME     CP_C1
#define CP_PFX      C1_
#include "../config.hpp"

#define CP_NAME     CP_C2
#define CP_PFX      C2_
#include "../config.hpp"

#define CP_NAME     CP_S0
#define CP_PFX      S0_
#define CP_SSE      1
#include "../config.hpp"

#define CP_NAME     CP_M0
#define CP_PFX      M0_
#include "../config_mix2.hpp"

#define CP_NAME     CP_M1
#define CP_PFX      M1_
#include "../config_mix2.hpp"

#define CP_NAME     CP_M2
#define CP_PFX      M2_
#include "../config_mix2.hpp"

#include "../sh_pupdater.inc"
#include "../sh_counter.inc"
#include "../sh_SSE.inc"
#include "../sh_mix2.inc"

typedef Counter<CP_C0> C0_Cell;
typedef Counter<CP_C1> C1_Cell;
typedef Counter<CP_C2> C2_Cell;
typedef Counter<CP_S0> SSE_Cell;
typedef SSE<CP_S0>     S0_SSE;
typedef Mix2<CP_M0>    Mix2_Cell;
typedef Mix2<CP_M1>    Mix2b_Cell;
typedef Mix2<CP_M2>    Mix2c_Cell;
#include "MOD/tc_model-C0_h.inc"
#include "MOD/tc_model-C1_h.inc"
#include "MOD/tc_model-C2_h.inc"
#include "MOD/tc_model-S0_h.inc"
#include "MOD/tc_model-M0_h.inc"
#include "MOD/tc_model-M1_h.inc"
#include "MOD/tc_model-M2_h.inc"

// ---- the model ----------------------------------------------------------

ALIGN(64) Rangecoder rc;
C0_T C0;
C1_T C1;
C2_T C2;
S0_T S0;
M0_T M0;
M1_T M1;
M2_T M2;
static double L = 0;   // ideal code length, bits

static void model_init() {
  C0.C0_Init(); for( qword i=0; i<qword(C0_Cx_Volume); i++ ) C0.C0_tbl[i].Init();
  C1.C1_Init(); for( qword i=0; i<qword(C1_Cx_Volume); i++ ) C1.C1_tbl[i].Init();
  C2.C2_Init(); for( qword i=0; i<qword(C2_Cx_Volume); i++ ) C2.C2_tbl[i].Init();
  M0.M0_Init(); for( qword i=0; i<qword(M0_Cx_Volume); i++ ) M0.M0_tbl[i].Init();
  M1.M1_Init(); for( qword i=0; i<qword(M1_Cx_Volume); i++ ) M1.M1_tbl[i].Init();
  M2.M2_Init(); for( qword i=0; i<qword(M2_Cx_Volume); i++ ) M2.M2_tbl[i].Init();
  S0.S0_Init(); for( qword i=0; i<qword(S0_Cx_Volume); i++ ) S0_SSE::Init( &S0.S0_tbl[ i*CP_S0::NB ] );
}
static void model_quit() {
  C0.C0_Quit(); C1.C1_Quit(); C2.C2_Quit(); M0.M0_Quit(); M1.M1_Quit(); M2.M2_Quit(); S0.S0_Quit();
}

#ifndef FINAL_MIX
#define FINAL_MIX 1
#endif

// cell/row indices of one binary decision
struct BitCx { uint c0, c1, c2, m0, m1, m2, s0; };

// one binary decision: p_m = mix2(C0, C1), p_n = mix2(p_m, C2),
// p = mix2'( p_n, SSE(p_n) )  (FINAL_MIX 0: p = SSE(p_n))
static int code_bit( int bit, const BitCx& x ) {
  SSE_Cell* sr = &S0.S0_tbl[ qword(x.s0) * CP_S0::NB ];
  Counter<CP_C0>::Pred pr0 = C0.C0_tbl[x.c0].PredictF();
  Counter<CP_C1>::Pred pr1 = C1.C1_tbl[x.c1].PredictF();
  Counter<CP_C2>::Pred pr2 = C2.C2_tbl[x.c2].PredictF();
  Mix2_Cell::Pred      pm  = M0.M0_tbl[x.m0].Mix( pr0.z, pr1.z );
  Mix2c_Cell::Pred     pn  = M2.M2_tbl[x.m2].Mix( pm.z, pr2.z );
  SSE_Pred<CP_S0>      ps  = S0_SSE::Predict( sr, pn.z );
#if FINAL_MIX
  Mix2b_Cell::Pred     pz  = M1.M1_tbl[x.m1].Mix( pn.z, ps.z );
  float zf = pz.z;
#else
  float zf = ps.z;
#endif
  float pf = sq( zf );                     // P(bit==0)
  uint p = uint( clamp( pf*float(SCALE) ) );
  bit = rc.rc_BProcess( p, bit );
  L -= log2( (bit ? double(SCALE-p) : double(p)) / SCALE );
  // A2 end-to-end errors, chained back through the stages (see coder0.cpp)
  float e_f = pf - float(1 - bit);
#if FINAL_MIX
  float e_s = e_f * pz.d2;                         // SSE output -> final
  float e_n = e_f * pz.d1 + e_s * ps.dzdz;         // p_n reaches the final directly and through the SSE
  M1.M1_tbl[x.m1].Update( bit, pz, e_f );
#else
  float e_s = e_f;
  float e_n = e_f * ps.dzdz;
#endif
  float e_m = e_n * pn.d1;
  C0.C0_tbl[x.c0].C_Update( bit, pr0, 1.0f, e_m * pm.d1 );
  C1.C1_tbl[x.c1].C_Update( bit, pr1, 1.0f, e_m * pm.d2 );
  C2.C2_tbl[x.c2].C_Update( bit, pr2, 1.0f, e_n * pn.d2 );
  M0.M0_tbl[x.m0].Update( bit, pm, e_m );
  M2.M2_tbl[x.m2].Update( bit, pn, e_n );
  S0_SSE::Update( sr, bit, ps, e_s );
  return bit;
}

static inline uint hash64( qword x ) {
  x ^= x >> 31; x *= 0x9E3779B97F4A7C15ULL; x ^= x >> 29; x *= 0xBF58476D1CE4E5B9ULL; x ^= x >> 32;
  return uint(x);
}
static inline int ilog2( qword m ) { int e = 0; while( m >>= 1 ) e++; return e; }

// sign + exponent bucket of a signed difference: 0 for 0, else
// 1 + [x<0] + 2*min(floor(log2|x|), 30)  (< 64)
static uint sbucket( long long x ) {
  if( x == 0 ) return 0;
  int e = ilog2( qword(x < 0 ? -x : x) ); if( e > 30 ) e = 30;
  return 1 + (x < 0) + 2*e;
}

struct Column {
  long long ref = 0;     // previous row's target
  uint zrun = 0;         // rows since the target last changed
  long long lastj = 0;   // last nonzero delta
  int ep = 40;           // exponent of the previous delta (40 = it was 0)
  uint dstate = 0;       // the previous delta's sbucket, for the other column
  long long pprev = 0;   // previous row's prediction
};

// side information of one row, from the non-target columns
struct RowSide { byte need, sch; long long pred[2]; };

// codes (or decodes) target v of column k
static long long code_value( int dec, long long v, int k, Column& c, const RowSide& rs, uint oth ) {
  long long d = dec ? 0 : v - c.ref;
  long long pk = rs.pred[k];
  // C2: the prediction and the current level in linear buckets of 2^PCX
  // (clipped to +-15), and how the prediction moved (PCX 0: the sign +
  // exponent bucket of prediction - level instead)
#ifndef PCX
#define PCX 10
#endif
#if PCX == 0
  uint pq = sbucket( pk - c.ref );
#else
  auto lin = []( long long x ) { long long q = x >> PCX; return uint( (q < -15 ? -15 : q > 15 ? 15 : q) + 16 ); };
  uint pq = lin( pk ) * 32 + lin( c.ref );
#endif
  uint dq = sbucket( pk - c.pprev );
  uint mq = sbucket( pk - c.ref );         // M2: the prediction vs. the current level
  qword base0 = hash64( (qword(rs.need)<<60) ^ (qword(k)<<56) ^ (qword(c.ep)<<48) );
  qword base1 = hash64( (qword(k)<<62) ^ (qword(oth)<<40) ^ qword(c.lastj) * 0x100000001B3ULL );
  qword base2 = hash64( (qword(k)<<62) ^ (qword(pq)<<48) ^ (qword(dq)<<40) ^ 0x5bd1e995ULL );
  auto bitc = [&]( int bit, qword node, int st, int e ) {
    BitCx x;
    x.c0 = C0_MakeCx( hash64( base0 ^ node ), c.zrun, rs.sch );
    x.c1 = C1_MakeCx( hash64( base1 ^ node * 0xD6E8FEB86659FD93ULL ) );
    x.c2 = C2_MakeCx( hash64( base2 ^ node * 0x9FB21C651E98DF25ULL ) );
    x.m0 = M0_MakeCx( k, st, e );
    x.m1 = M1_MakeCx( k, st, e );
    x.m2 = M2_MakeCx( k, st, mq );
    x.s0 = S0_MakeCx( k, st, c.zrun );
    return code_bit( bit, x );
  };
  int z = bitc( d == 0, 1, 0, 0 );
  if( !z ) {
    int neg = bitc( d < 0, 2, 1, 0 );
    qword m = qword( d < 0 ? -d : d );
    int e = dec ? 0 : ilog2( m );
    int j = 0;
    for( ; j < 40; j++ ) {                 // unary exponent, |d| < 2^33
      int more = bitc( dec ? 0 : e > j, 16 + j, 2, j );
      if( !more ) break;
    }
    e = j;
    qword t = 1;                           // bit tree over the mantissa
    for( int i = e-1; i >= 0; i-- ) {
      int b = bitc( dec ? 0 : int((m >> i) & 1), (qword(1)<<40) ^ (qword(e)<<34) ^ t, 3, e );
      t = 2*t + b;
    }
    m = t;
    if( dec ) d = neg ? -(long long)m : (long long)m;
    c.lastj = d;
    c.zrun = 0;
    c.ep = e;
  } else {
    d = 0;
    if( c.zrun < 0xFFFFFFFF ) c.zrun++;
    c.ep = 40;
  }
  c.dstate = sbucket( d );
  c.pprev = pk;
  c.ref += d;
  return c.ref;
}

// ---- TSV handling -------------------------------------------------------

static void die( const char* fmt, ... ) {
  va_list ap; va_start( ap, fmt );
  fprintf( stderr, "error: " ); vfprintf( stderr, fmt, ap ); fprintf( stderr, "\n" );
  va_end( ap ); exit( 1 );
}

static std::string read_file( const char* fn ) {
  FILE* f = fopen( fn, "rb" ); if( !f ) die( "can't open %s", fn );
  std::string s; char buf[1<<16]; size_t n;
  while( (n = fread( buf, 1, sizeof buf, f )) > 0 ) s.append( buf, n );
  fclose( f );
  return s;
}
static FILE* wopen( const char* fn ) { FILE* f = fopen( fn, "wb" ); if( !f ) die( "can't create %s", fn ); return f; }
static void write_file( const char* fn, const std::string& s ) {
  FILE* f = wopen( fn ); if( fwrite( s.data(), 1, s.size(), f ) != s.size() ) die( "can't write %s", fn ); fclose( f );
}

typedef std::pair<const char*,size_t> Field;
// one line: its fields (tab-separated, without '\r' and '\n'), and whether
// it ended in "\r" / "\n"
struct Line { std::vector<Field> f; bool cr, nl; };
static std::vector<Line> split_lines( const std::string& s ) {
  std::vector<Line> r;
  size_t i = 0;
  while( i < s.size() ) {
    size_t j = s.find( '\n', i );
    Line l; l.nl = j != std::string::npos;
    if( !l.nl ) j = s.size();
    const char* p = s.data()+i; const char* e = s.data()+j;
    l.cr = e > p && e[-1] == '\r'; if( l.cr ) e--;
    for(;;) {
      const char* t = (const char*)memchr( p, '\t', e-p );
      if( !t ) { l.f.push_back( { p, size_t(e-p) } ); break; }
      l.f.push_back( { p, size_t(t-p) } ); p = t+1;
    }
    r.push_back( std::move(l) );
    i = j+1;
  }
  return r;
}
static void put_line( std::string& o, const Line& l, int skip0 = -1, int skip1 = -1 ) {
  bool first = true;
  for( size_t i = 0; i < l.f.size(); i++ ) {
    if( int(i) == skip0 || int(i) == skip1 ) continue;
    if( !first ) o += '\t';
    o.append( l.f[i].first, l.f[i].second ); first = false;
  }
  if( l.cr ) o += '\r';
  if( l.nl ) o += '\n';
}
static std::string colname( const Field& f ) {
  std::string s( f.first, f.second );
  size_t k = s.find( '/' ); if( k != std::string::npos ) s.resize( k );
  return s;
}

// Side information from the table without the targets (header + rows); the
// same whether it comes from input.tsv or output.tsv.
static std::vector<RowSide> side_info( const std::vector<Line>& rest ) {
  std::vector<RowSide> s;
  if( rest.empty() ) return s;
  const std::vector<Field>& h = rest[0].f;
  int cneed = -1, cpred[2] = {-1, -1}; std::vector<int> grp[3];
  for( size_t i = 0; i < h.size(); i++ ) {
    std::string n = colname( h[i] );
    if( n == "need_prediction" ) cneed = int(i);
    else if( n == "pred_t0" ) cpred[0] = int(i);
    else if( n == "pred_t1" ) cpred[1] = int(i);
    else if( n.compare( 0, 4, "i0_p" ) == 0 ) grp[0].push_back( int(i) );
    else if( n.compare( 0, 4, "i0_v" ) == 0 ) grp[1].push_back( int(i) );
    else if( n.compare( 0, 4, "i0_d" ) == 0 ) grp[2].push_back( int(i) );
  }
  for( size_t r = 1; r < rest.size(); r++ ) {
    const std::vector<Field>& f = rest[r].f;
    RowSide x; x.need = 1; x.sch = 0;
    if( cneed >= 0 && size_t(cneed) < f.size() ) x.need = !(f[cneed].second == 1 && f[cneed].first[0] == '0');
    for( int k = 0; k < 2; k++ ) {
      x.pred[k] = 0;
      if( cpred[k] >= 0 && size_t(cpred[k]) < f.size() && f[cpred[k]].second < 24 ) {
        char buf[24]; memcpy( buf, f[cpred[k]].first, f[cpred[k]].second ); buf[f[cpred[k]].second] = 0;
        x.pred[k] = strtoll( buf, 0, 10 );
      }
    }
    if( r > 1 ) {
      const std::vector<Field>& pf = rest[r-1].f;
      for( int g = 0; g < 3; g++ )
        for( int c : grp[g] ) {
          if( size_t(c) >= f.size() || size_t(c) >= pf.size() ) continue;
          if( f[c].second != pf[c].second || memcmp( f[c].first, pf[c].first, f[c].second ) ) { x.sch |= 1<<g; break; }
        }
    }
    s.push_back( x );
  }
  return s;
}

static int32_t parse_int( const Field& fl, size_t row, const char* fn ) {
  char buf[32];
  if( fl.second == 0 || fl.second > 11 ) die( "%s: row %zu: bad integer in a target column", fn, row );
  memcpy( buf, fl.first, fl.second ); buf[fl.second] = 0;
  char* e; long long v = strtoll( buf, &e, 10 );
  char chk[32]; snprintf( chk, sizeof chk, "%lld", v );
  if( *e || v < INT32_MIN || v > INT32_MAX || strcmp( chk, buf ) )
    die( "%s: row %zu: '%s' is not a canonical int32", fn, row, buf );
  return int32_t(v);
}

// the targets and where they go
struct Targets {
  std::string name[2];
  int pos[2];                   // column positions in input.tsv
  std::vector<int32_t> v[2];
};

// input.tsv -> targets + the text of output.tsv
static std::string split_input( const std::string& s, const char* fn, Targets& t ) {
  std::vector<Line> lines = split_lines( s );
  if( lines.empty() ) die( "%s: empty file", fn );
  const std::vector<Field>& h = lines[0].f;
  t.pos[0] = t.pos[1] = -1;
  for( size_t i = 0; i < h.size(); i++ ) {
    std::string n = colname( h[i] );
    for( int k = 0; k < 2; k++ )
      if( n == (k ? "t1" : "t0") ) {
        if( t.pos[k] >= 0 ) die( "%s: two t%d columns", fn, k );
        t.pos[k] = int(i); t.name[k].assign( h[i].first, h[i].second );
      }
  }
  if( t.pos[0] < 0 || t.pos[1] < 0 ) die( "%s: needs columns t0 and t1", fn );
  if( h.size() < 3 ) die( "%s: needs at least one column besides t0 and t1", fn );
  if( t.pos[0] > 0xFFFF || t.pos[1] > 0xFFFF ) die( "%s: too many columns", fn );
  std::string rest;
  for( size_t i = 0; i < lines.size(); i++ ) {
    if( lines[i].f.size() != h.size() ) die( "%s: line %zu has %zu columns, the header %zu", fn, i+1, lines[i].f.size(), h.size() );
    if( i ) for( int k = 0; k < 2; k++ ) t.v[k].push_back( parse_int( lines[i].f[t.pos[k]], i, fn ) );
    put_line( rest, lines[i], t.pos[0], t.pos[1] );
  }
  return rest;
}

// output.tsv + targets -> restored.tsv
static void write_restored( const std::vector<Line>& rest, const Targets& t, const char* fn ) {
  size_t nc = rest[0].f.size() + 2;
  for( int k = 0; k < 2; k++ )
    if( t.pos[k] < 0 || size_t(t.pos[k]) >= nc || t.pos[0] == t.pos[1] ) die( "bad target column positions" );
  std::string o;
  char b[16];
  for( size_t r = 0; r < rest.size(); r++ ) {
    const Line& l = rest[r];
    if( l.f.size() + 2 != nc ) die( "line %zu has %zu columns, the header %zu", r+1, l.f.size(), nc-2 );
    for( size_t i = 0, j = 0; i < nc; i++ ) {
      if( i ) o += '\t';
      int k = int(i) == t.pos[0] ? 0 : int(i) == t.pos[1] ? 1 : -1;
      if( k < 0 ) { o.append( l.f[j].first, l.f[j].second ); j++; }
      else if( r == 0 ) o += t.name[k];
      else { snprintf( b, sizeof b, "%d", t.v[k][r-1] ); o += b; }
    }
    if( l.cr ) o += '\r';
    if( l.nl ) o += '\n';
  }
  write_file( fn, o );
}

static void put32( std::string& o, uint x ) { for( int i=0; i<4; i++ ) o += char(byte(x >> 8*i)); }
static uint get32( const std::string& s, size_t& pos, const char* fn ) {
  if( s.size() - pos < 4 ) die( "%s: truncated", fn );
  uint x = 0; for( int i=0; i<4; i++ ) x |= uint(byte(s[pos++])) << 8*i;
  return x;
}

// compress (dec=0) or decompress (dec=1) the targets with the model
static void run_model( int dec, Targets& t, const std::vector<RowSide>& side ) {
  model_init();
  Column c[2];
  size_t n = side.size();
  if( dec ) { t.v[0].assign( n, 0 ); t.v[1].assign( n, 0 ); }
  for( size_t r = 0; r < n; r++ ) {
    long long a = code_value( dec, t.v[0][r], 0, c[0], side[r], c[1].dstate );
    long long b = code_value( dec, t.v[1][r], 1, c[1], side[r], c[0].dstate );
    if( dec ) {
      if( a < INT32_MIN || a > INT32_MAX || b < INT32_MIN || b > INT32_MAX ) die( "corrupt stream" );
      t.v[0][r] = int32_t(a); t.v[1][r] = int32_t(b);
    }
  }
  model_quit();
}

// ---- modes ----------------------------------------------------------------

int main( int argc, char** argv ) {
#if defined(__SSE3__) || defined(__x86_64__) || defined(_M_X64)
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
  std::string mode = argc > 1 ? argv[1] : "";
  if( (mode == "c" && (argc == 4 || argc == 5)) || ((mode == "tc" || mode == "bc") && argc == 5) ) {
    std::string in = read_file( argv[2] );
    Targets t;
    std::string rest = split_input( in, argv[2], t );
    if( argc == 5 ) write_file( argv[3], rest );
    size_t n = t.v[0].size();
    std::string o;
    if( mode == "tc" ) {
      char b[32];
      for( size_t i = 0; i < n; i++ ) { snprintf( b, sizeof b, "%d\t%d\n", t.v[0][i], t.v[1][i] ); o += b; }
      write_file( argv[4], o );
      return 0;
    }
    if( mode == "bc" ) {
      for( size_t i = 0; i < n; i++ ) { put32( o, uint(t.v[0][i]) ); put32( o, uint(t.v[1][i]) ); }
      write_file( argv[4], o );
      return 0;
    }
    const char* ofn = argv[argc-1];
    FILE* g = wopen( ofn );
    o = "TSC2"; put32( o, uint(n) );
    for( int k = 0; k < 2; k++ ) {
      if( t.name[k].size() > 255 ) die( "column name too long" );
      o += char(t.pos[k] & 255); o += char(t.pos[k] >> 8);
      o += char(t.name[k].size()); o += t.name[k];
    }
    fwrite( o.data(), 1, o.size(), g );
    rc.StartEncode( g );
    run_model( 0, t, side_info( split_lines( rest ) ) );
    rc.FinishEncode();
    long sz = ftell( g ); fclose( g );
    fprintf( stderr, "%s: %zu rows -> %s %ld bytes (%.3f bits/target, model %.0f bytes)\n",
             argv[2], n, ofn, sz, 8.0*sz/(2.0*(n ? n : 1)), L/8 );
    return 0;
  }
  if( (mode == "d" || mode == "td" || mode == "bd") && argc == 5 ) {
    std::string rs = read_file( argv[2] ), in = read_file( argv[3] );
    std::vector<Line> rest = split_lines( rs );
    if( rest.empty() ) die( "%s: empty file", argv[2] );
    size_t n = rest.size() - 1;
    Targets t;
    if( mode == "d" ) {
      size_t pos = 0;
      if( in.size() < 4 || memcmp( in.data(), "TSC2", 4 ) ) die( "%s: not a tsvcomp stream", argv[3] );
      pos = 4;
      uint nr = get32( in, pos, argv[3] );
      if( nr != n ) die( "%s has %zu rows, %s has %u", argv[2], n, argv[3], nr );
      for( int k = 0; k < 2; k++ ) {
        if( in.size() - pos < 3 ) die( "%s: truncated", argv[3] );
        t.pos[k] = byte(in[pos]) | byte(in[pos+1]) << 8; pos += 2;
        size_t len = byte(in[pos++]);
        if( in.size() - pos < len ) die( "%s: truncated", argv[3] );
        t.name[k].assign( in, pos, len ); pos += len;
      }
      FILE* f = fopen( argv[3], "rb" ); fseek( f, long(pos), SEEK_SET );
      rc.StartDecode( f );
      run_model( 1, t, side_info( rest ) );
      fclose( f );
    } else {
      // no header in cols.*: t0, t1 become the last two columns, with the
      // scale suffix of the first scaled column of output.tsv
      std::string suf;
      for( const Field& x : rest[0].f ) {
        std::string s( x.first, x.second ); size_t k = s.find( '/' );
        if( k != std::string::npos ) { suf = s.substr( k ); break; }
      }
      int nc = int(rest[0].f.size());
      for( int k = 0; k < 2; k++ ) { t.name[k] = (k ? "t1" : "t0") + suf; t.pos[k] = nc + k; }
      if( mode == "bd" ) {
        if( in.size() != n*8 ) die( "%s: %zu bytes, expected %zu for %zu rows", argv[3], in.size(), n*8, n );
        size_t pos = 0;
        for( size_t i = 0; i < n; i++ ) for( int k = 0; k < 2; k++ ) t.v[k].push_back( int32_t(get32( in, pos, argv[3] )) );
      } else {
        std::vector<Line> l = split_lines( in );
        if( l.size() != n ) die( "%s has %zu lines, %s has %zu rows", argv[3], l.size(), argv[2], n );
        for( size_t i = 0; i < n; i++ ) {
          if( l[i].f.size() != 2 ) die( "%s: line %zu: expected two columns", argv[3], i+1 );
          for( int k = 0; k < 2; k++ ) t.v[k].push_back( parse_int( l[i].f[k], i+1, argv[3] ) );
        }
      }
    }
    write_restored( rest, t, argv[4] );
    return 0;
  }
  fprintf( stderr,
    "tsvcomp -- the target columns t0, t1 of a TSV, coded with the other columns as context\n"
    "  tsvcomp c  input.tsv output.tsv output.rc    compress (output.tsv = the other columns)\n"
    "  tsvcomp c  input.tsv output.rc               compress, no output.tsv (for IDX/opt.pl)\n"
    "  tsvcomp d  output.tsv output.rc restored.tsv\n"
    "  tsvcomp tc input.tsv output.tsv cols.tsv     t0, t1 as text, no header\n"
    "  tsvcomp td output.tsv cols.tsv restored.tsv\n"
    "  tsvcomp bc input.tsv output.tsv cols.bin     t0, t1 as int32 LE pairs\n"
    "  tsvcomp bd output.tsv cols.bin restored.tsv\n" );
  return 1;
}
