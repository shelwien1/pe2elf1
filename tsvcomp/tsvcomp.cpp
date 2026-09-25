// tsvcomp.cpp -- compressor for the first two (integer) columns of a TSV,
// e.g. pred_t0/pred_t1 of connectome/quantize output, using the rest of the
// row as side information.  Built from coder0's model: every bit goes
// through p = mix2( p_m, SSE(p_m) ), p_m = mix2( C0, C1 ) of the adaptive counter tables
// (../sh_counter.inc, ../sh_mix2.inc, ../sh_SSE.inc), with tsvcomp's own
// contexts (IDX/tc_model-*.idx).
//
//   tsvcomp c  input.tsv output.tsv output.rc   first two columns -> output.rc (compressed),
//                                               the rest of the table -> output.tsv
//   tsvcomp c  input.tsv output.rc              the same without writing output.tsv
//                                               (the form IDX/opt.pl runs: "exe c in out")
//   tsvcomp d  output.tsv output.rc restored.tsv
//   tsvcomp tc input.tsv output.tsv cols.tsv    first two columns as text, no header
//   tsvcomp td output.tsv cols.tsv restored.tsv
//   tsvcomp bc input.tsv output.tsv cols.bin    first two columns as int32 LE pairs, no header
//   tsvcomp bd output.tsv cols.bin restored.tsv
//
// restored.tsv is byte-identical to input.tsv.  The two columns must hold
// canonical int32 decimals ("-12", not "+12" or "012").  tc/bc keep no
// header: td/bd name the columns pred_t0, pred_t1 with the scale suffix of
// output.tsv's first scaled column ("/10000"), as quantize writes them.
//
// .rc format: "TSC1", u32 row count (without the header), then for each of
// the two columns u8 name length + name, then the range coder stream.
//
// Binarization of a value v of column k (k = 0, 1; row by row, t0 before t1):
//   d = v - (previous row's value of column k)
//   zero flag [d == 0]; sign [d < 0]; e = floor(log2 |d|) in unary
//   ([e > j] for j = 0, 1, ..); the e bits of |d| below its leading 1, MSB
//   first, each in the context of the bits above it (a bit tree: recurring
//   jump sizes are learned exactly).
// Contexts (IDX/tc_model-*.idx):
//   C0  hash(need_prediction, k, node, e of the previous delta)
//       x rows since column k last changed x side-info change flags
//   C1  hash(k, node, column k's last nonzero delta, the other column's
//       delta: this row's for t1, the previous row's for t0)
//   M0  k x stage (zero/sign/exponent/mantissa) x exponent (step)
//   M1  (final mixer of p_m and SSE(p_m)) same context as M0
//   S0  k x stage x rows since the last change
// The side-info change flags compare this row with the previous one in the
// i0 price-like (i0_p*), volume-like (i0_v*) and trade (i0_dp*/i0_dv*) columns.

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
#include "MOD/tc_model-S0_p.inc"
#include "MOD/tc_model-M0_p.inc"
#include "MOD/tc_model-M1_p.inc"

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

#include "../sh_pupdater.inc"
#include "../sh_counter.inc"
#include "../sh_SSE.inc"
#include "../sh_mix2.inc"

typedef Counter<CP_C0> C0_Cell;
typedef Counter<CP_C1> C1_Cell;
typedef Counter<CP_S0> SSE_Cell;
typedef SSE<CP_S0>     S0_SSE;
typedef Mix2<CP_M0>    Mix2_Cell;
typedef Mix2<CP_M1>    Mix2b_Cell;
#include "MOD/tc_model-C0_h.inc"
#include "MOD/tc_model-C1_h.inc"
#include "MOD/tc_model-S0_h.inc"
#include "MOD/tc_model-M0_h.inc"
#include "MOD/tc_model-M1_h.inc"

// ---- the model ----------------------------------------------------------

ALIGN(64) Rangecoder rc;
C0_T C0;
C1_T C1;
S0_T S0;
M0_T M0;
M1_T M1;
static double L = 0;   // ideal code length, bits

static void model_init() {
  C0.C0_Init(); for( qword i=0; i<qword(C0_Cx_Volume); i++ ) C0.C0_tbl[i].Init();
  C1.C1_Init(); for( qword i=0; i<qword(C1_Cx_Volume); i++ ) C1.C1_tbl[i].Init();
  M0.M0_Init(); for( qword i=0; i<qword(M0_Cx_Volume); i++ ) M0.M0_tbl[i].Init();
  M1.M1_Init(); for( qword i=0; i<qword(M1_Cx_Volume); i++ ) M1.M1_tbl[i].Init();
  S0.S0_Init(); for( qword i=0; i<qword(S0_Cx_Volume); i++ ) S0_SSE::Init( &S0.S0_tbl[ i*CP_S0::NB ] );
}
static void model_quit() { C0.C0_Quit(); C1.C1_Quit(); M0.M0_Quit(); M1.M1_Quit(); S0.S0_Quit(); }

// one binary decision.  FINAL_MIX 1 (default): p = mix2'( p_m, SSE(p_m) ),
// p_m = mix2( C0, C1 ) -- the final mixer M1 blends the mixed prediction with
// its SSE refinement.  FINAL_MIX 0: p = SSE( p_m ), as in coder0.
#ifndef FINAL_MIX
#define FINAL_MIX 1
#endif
static int code_bit( int bit, uint ox, uint oy, uint mx, uint fx, uint sx ) {
  SSE_Cell* sr = &S0.S0_tbl[ qword(sx) * CP_S0::NB ];
  Counter<CP_C0>::Pred pr0 = C0.C0_tbl[ox].PredictF();
  Counter<CP_C1>::Pred pr1 = C1.C1_tbl[oy].PredictF();
  Mix2_Cell::Pred      pm  = M0.M0_tbl[mx].Mix( pr0.z, pr1.z );
  SSE_Pred<CP_S0>      ps  = S0_SSE::Predict( sr, pm.z );
#if FINAL_MIX
  Mix2b_Cell&          fm  = M1.M1_tbl[fx];
  Mix2b_Cell::Pred     pz  = fm.Mix( pm.z, ps.z );
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
  float e_m = e_f * pz.d1 + e_s * ps.dzdz;         // p_m reaches the final directly and through the SSE
  fm.Update( bit, pz, e_f );
#else
  float e_s = e_f;
  float e_m = e_f * ps.dzdz;
#endif
  C0.C0_tbl[ox].C_Update( bit, pr0, 1.0f, e_m * pm.d1 );
  C1.C1_tbl[oy].C_Update( bit, pr1, 1.0f, e_m * pm.d2 );
  M0.M0_tbl[mx].Update( bit, pm, e_m );
  S0_SSE::Update( sr, bit, ps, e_s );
  return bit;
}

static inline uint hash64( qword x ) {
  x ^= x >> 31; x *= 0x9E3779B97F4A7C15ULL; x ^= x >> 29; x *= 0xBF58476D1CE4E5B9ULL; x ^= x >> 32;
  return uint(x);
}
static inline int ilog2( qword m ) { int e = 0; while( m >>= 1 ) e++; return e; }

struct Column {
  long long ref = 0;     // previous row's value
  uint zrun = 0;         // rows since the value last changed
  long long lastj = 0;   // last nonzero delta
  int ep = 40;           // exponent of the previous delta (40 = it was 0)
  uint dstate = 0;       // the previous delta as (zero, sign, e), for the other column
};

static uint dstate( long long d ) {
  if( d == 0 ) return 0;
  return 1 + (d < 0) + 2*ilog2( qword(d < 0 ? -d : d) );
}

// codes (or decodes) value v of column k; need/sch = side info of the row;
// oth = the other column's delta state
static long long code_value( int dec, long long v, int k, Column& c, int need, uint sch, uint oth ) {
  long long d = dec ? 0 : v - c.ref;
  qword base0 = hash64( (qword(need)<<60) ^ (qword(k)<<56) ^ (qword(c.ep)<<48) ) ;
  qword base1 = hash64( (qword(k)<<62) ^ (qword(oth)<<40) ^ qword(c.lastj) * 0x100000001B3ULL );
  auto bitc = [&]( int bit, qword node, int st, int e ) {
    uint ox = C0_MakeCx( hash64( base0 ^ node ), c.zrun, sch );
    uint oy = C1_MakeCx( hash64( base1 ^ node * 0xD6E8FEB86659FD93ULL ) );
    uint mx = M0_MakeCx( k, st, e );
    uint fx = M1_MakeCx( k, st, e );
    uint sx = S0_MakeCx( k, st, c.zrun );
    return code_bit( bit, ox, oy, mx, fx, sx );
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
  c.dstate = dstate( d );
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

struct Line { const char* p; size_t n; bool nl; };   // without the '\n'; nl = it had one
static std::vector<Line> split_lines( const std::string& s ) {
  std::vector<Line> r;
  size_t i = 0;
  while( i < s.size() ) {
    size_t j = s.find( '\n', i );
    if( j == std::string::npos ) { r.push_back( { s.data()+i, s.size()-i, false } ); break; }
    r.push_back( { s.data()+i, j-i, true } );
    i = j+1;
  }
  return r;
}

// Side information from the stripped table (header + rows): need_prediction
// and the change flags; the same whether it comes from input.tsv or output.tsv.
struct Side { std::vector<byte> need, sch; };
static std::string colname( const char* p, size_t n ) {
  std::string s( p, n );
  size_t k = s.find( '/' ); if( k != std::string::npos ) s.resize( k );
  if( !s.empty() && s.back() == '\r' ) s.pop_back();
  return s;
}
static void fields( const Line& l, std::vector<std::pair<const char*,size_t>>& f ) {
  f.clear();
  const char* p = l.p; const char* e = l.p + l.n;
  if( e > p && e[-1] == '\r' ) e--;
  for(;;) {
    const char* t = (const char*)memchr( p, '\t', e-p );
    if( !t ) { f.push_back( { p, size_t(e-p) } ); break; }
    f.push_back( { p, size_t(t-p) } ); p = t+1;
  }
}
static Side side_info( const std::vector<Line>& rest ) {
  Side s;
  if( rest.empty() ) return s;
  std::vector<std::pair<const char*,size_t>> h, f, pf;
  fields( rest[0], h );
  int cneed = -1; std::vector<int> grp[3];
  for( size_t i = 0; i < h.size(); i++ ) {
    std::string n = colname( h[i].first, h[i].second );
    if( n == "need_prediction" ) cneed = int(i);
    else if( n.compare( 0, 4, "i0_p" ) == 0 ) grp[0].push_back( int(i) );
    else if( n.compare( 0, 4, "i0_v" ) == 0 ) grp[1].push_back( int(i) );
    else if( n.compare( 0, 4, "i0_d" ) == 0 ) grp[2].push_back( int(i) );
  }
  for( size_t r = 1; r < rest.size(); r++ ) {
    fields( rest[r], f );
    byte need = 1, ch = 0;
    if( cneed >= 0 && size_t(cneed) < f.size() ) need = !(f[cneed].second == 1 && f[cneed].first[0] == '0');
    if( r > 1 )
      for( int g = 0; g < 3; g++ )
        for( int c : grp[g] ) {
          if( size_t(c) >= f.size() || size_t(c) >= pf.size() ) continue;
          if( f[c].second != pf[c].second || memcmp( f[c].first, pf[c].first, f[c].second ) ) { ch |= 1<<g; break; }
        }
    s.need.push_back( need ); s.sch.push_back( ch );
    pf.swap( f );
  }
  return s;
}

// input.tsv -> names of the two columns, their values, the rest of each line
struct Split {
  std::string n0, n1;
  std::vector<int32_t> v0, v1;
  std::vector<Line> rest;
};
static int32_t parse_int( const char* p, size_t n, size_t row, const char* fn ) {
  char buf[32];
  if( n == 0 || n > 11 ) die( "%s: row %zu: bad integer in the first two columns", fn, row );
  memcpy( buf, p, n ); buf[n] = 0;
  char* e; long long v = strtoll( buf, &e, 10 );
  char chk[32]; snprintf( chk, sizeof chk, "%lld", v );
  if( *e || v < INT32_MIN || v > INT32_MAX || strcmp( chk, buf ) )
    die( "%s: row %zu: '%s' is not a canonical int32", fn, row, buf );
  return int32_t(v);
}
static Split split_input( const std::string& s, const char* fn ) {
  Split r;
  std::vector<Line> lines = split_lines( s );
  if( lines.empty() ) die( "%s: empty file", fn );
  for( size_t i = 0; i < lines.size(); i++ ) {
    const Line& l = lines[i];
    const char* t1 = (const char*)memchr( l.p, '\t', l.n );
    const char* t2 = t1 ? (const char*)memchr( t1+1, '\t', l.p+l.n-(t1+1) ) : 0;
    if( !t2 ) die( "%s: line %zu: needs at least 3 columns", fn, i+1 );
    r.rest.push_back( { t2+1, size_t(l.p+l.n-(t2+1)), l.nl } );
    if( i == 0 ) { r.n0.assign( l.p, t1-l.p ); r.n1.assign( t1+1, t2-(t1+1) ); continue; }
    r.v0.push_back( parse_int( l.p, t1-l.p, i, fn ) );
    r.v1.push_back( parse_int( t1+1, t2-(t1+1), i, fn ) );
  }
  return r;
}
static void write_rest( const Split& s, const char* fn ) {
  FILE* f = wopen( fn );
  for( const Line& l : s.rest ) { fwrite( l.p, 1, l.n, f ); if( l.nl ) putc( '\n', f ); }
  fclose( f );
}
static void write_restored( const Split& s, const char* fn ) {
  FILE* f = wopen( fn );
  for( size_t i = 0; i < s.rest.size(); i++ ) {
    if( i == 0 ) fprintf( f, "%s\t%s\t", s.n0.c_str(), s.n1.c_str() );
    else fprintf( f, "%d\t%d\t", s.v0[i-1], s.v1[i-1] );
    fwrite( s.rest[i].p, 1, s.rest[i].n, f ); if( s.rest[i].nl ) putc( '\n', f );
  }
  fclose( f );
}
// output.tsv -> Split without the values (names inferred for tc/bc)
static Split load_rest( const std::string& s, const char* fn ) {
  Split r;
  r.rest = split_lines( s );
  if( r.rest.empty() ) die( "%s: empty file", fn );
  std::vector<std::pair<const char*,size_t>> h;
  fields( r.rest[0], h );
  std::string suf;
  for( auto& x : h ) {
    std::string n( x.first, x.second );
    size_t k = n.find( '/' );
    if( k != std::string::npos ) { suf = n.substr( k ); break; }
  }
  r.n0 = "pred_t0" + suf; r.n1 = "pred_t1" + suf;
  return r;
}

static void put32( FILE* f, uint x ) { for( int i=0; i<4; i++ ) putc( byte(x >> 8*i), f ); }
static uint get32( const std::string& s, size_t& pos, const char* fn ) {
  if( s.size() - pos < 4 ) die( "%s: truncated", fn );
  uint x = 0; for( int i=0; i<4; i++ ) x |= uint(byte(s[pos++])) << 8*i;
  return x;
}

// ---- modes ----------------------------------------------------------------

// compress (dec=0) or decompress (dec=1) the two columns of s with the model
static void run_model( int dec, Split& s, const Side& side ) {
  model_init();
  Column c[2];
  size_t n = s.rest.size() - 1;
  if( dec ) { s.v0.resize( n ); s.v1.resize( n ); }
  for( size_t r = 0; r < n; r++ ) {
    int need = side.need.size() > r ? side.need[r] : 1;
    uint sch = side.sch.size() > r ? side.sch[r] : 0;
    long long a = code_value( dec, s.v0[r], 0, c[0], need, sch, c[1].dstate );
    long long b = code_value( dec, s.v1[r], 1, c[1], need, sch, c[0].dstate );
    if( dec ) {
      if( a < INT32_MIN || a > INT32_MAX || b < INT32_MIN || b > INT32_MAX ) die( "corrupt stream" );
      s.v0[r] = int32_t(a); s.v1[r] = int32_t(b);
    }
  }
  model_quit();
}

int main( int argc, char** argv ) {
#if defined(__SSE3__) || defined(__x86_64__) || defined(_M_X64)
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
  std::string mode = argc > 1 ? argv[1] : "";
  if( mode == "c" && (argc == 4 || argc == 5) ) {
    std::string in = read_file( argv[2] );
    Split s = split_input( in, argv[2] );
    if( argc == 5 ) write_rest( s, argv[3] );
    const char* ofn = argv[argc-1];
    FILE* g = wopen( ofn );
    fwrite( "TSC1", 1, 4, g ); put32( g, uint(s.v0.size()) );
    for( const std::string* n : { &s.n0, &s.n1 } ) {
      if( n->size() > 255 ) die( "column name too long" );
      putc( int(n->size()), g ); fwrite( n->data(), 1, n->size(), g );
    }
    rc.StartEncode( g );
    run_model( 0, s, side_info( s.rest ) );
    rc.FinishEncode();
    long sz = ftell( g ); fclose( g );
    fprintf( stderr, "%s: %zu rows -> %s %ld bytes (%.3f bits/value, model %.0f bytes)\n",
             argv[2], s.v0.size(), ofn, sz, 8.0*sz/(2.0*(s.v0.size() ? s.v0.size() : 1)), L/8 );
    return 0;
  }
  if( mode == "d" && argc == 5 ) {
    std::string rest = read_file( argv[2] ), in = read_file( argv[3] );
    Split s = load_rest( rest, argv[2] );
    size_t pos = 0;
    if( in.size() < 4 || memcmp( in.data(), "TSC1", 4 ) ) die( "%s: not a tsvcomp stream", argv[3] );
    pos = 4;
    uint n = get32( in, pos, argv[3] );
    if( n != s.rest.size() - 1 ) die( "%s has %zu rows, %s has %u", argv[2], s.rest.size()-1, argv[3], n );
    for( std::string* nm : { &s.n0, &s.n1 } ) {
      if( pos >= in.size() ) die( "%s: truncated", argv[3] );
      size_t k = byte(in[pos++]);
      if( in.size() - pos < k ) die( "%s: truncated", argv[3] );
      nm->assign( in, pos, k ); pos += k;
    }
    FILE* f = fopen( argv[3], "rb" ); fseek( f, long(pos), SEEK_SET );
    rc.StartDecode( f );
    run_model( 1, s, side_info( s.rest ) );
    fclose( f );
    write_restored( s, argv[4] );
    return 0;
  }
  if( (mode == "tc" || mode == "bc") && argc == 5 ) {
    std::string in = read_file( argv[2] );
    Split s = split_input( in, argv[2] );
    write_rest( s, argv[3] );
    FILE* g = wopen( argv[4] );
    for( size_t i = 0; i < s.v0.size(); i++ )
      if( mode == "tc" ) fprintf( g, "%d\t%d\n", s.v0[i], s.v1[i] );
      else { put32( g, uint(s.v0[i]) ); put32( g, uint(s.v1[i]) ); }
    fclose( g );
    return 0;
  }
  if( (mode == "td" || mode == "bd") && argc == 5 ) {
    std::string rest = read_file( argv[2] ), in = read_file( argv[3] );
    Split s = load_rest( rest, argv[2] );
    size_t n = s.rest.size() - 1;
    if( mode == "bd" ) {
      if( in.size() != n*8 ) die( "%s: %zu bytes, expected %zu for %zu rows", argv[3], in.size(), n*8, n );
      size_t pos = 0;
      for( size_t i = 0; i < n; i++ ) { s.v0.push_back( int32_t(get32( in, pos, argv[3] )) ); s.v1.push_back( int32_t(get32( in, pos, argv[3] )) ); }
    } else {
      std::vector<Line> l = split_lines( in );
      if( l.size() != n ) die( "%s has %zu lines, %s has %zu rows", argv[3], l.size(), argv[2], n );
      for( size_t i = 0; i < n; i++ ) {
        const char* t = (const char*)memchr( l[i].p, '\t', l[i].n );
        if( !t ) die( "%s: line %zu: expected two columns", argv[3], i+1 );
        s.v0.push_back( parse_int( l[i].p, t-l[i].p, i+1, argv[3] ) );
        s.v1.push_back( parse_int( t+1, l[i].p+l[i].n-(t+1), i+1, argv[3] ) );
      }
    }
    write_restored( s, argv[4] );
    return 0;
  }
  fprintf( stderr,
    "tsvcomp -- the first two integer columns of a TSV, coded with SSE(mix2(C0,C1))\n"
    "  tsvcomp c  input.tsv output.tsv output.rc    compress (output.tsv = the other columns)\n"
    "  tsvcomp c  input.tsv output.rc               compress, no output.tsv (for IDX/opt.pl)\n"
    "  tsvcomp d  output.tsv output.rc restored.tsv\n"
    "  tsvcomp tc input.tsv output.tsv cols.tsv     the two columns as text, no header\n"
    "  tsvcomp td output.tsv cols.tsv restored.tsv\n"
    "  tsvcomp bc input.tsv output.tsv cols.bin     the two columns as int32 LE pairs\n"
    "  tsvcomp bd output.tsv cols.bin restored.tsv\n" );
  return 1;
}
