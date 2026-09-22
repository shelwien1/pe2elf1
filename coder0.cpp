
// -------------------------------------------------------------
// LOGWR+UVROT port (2026-07-15): wr optimized in log space, in the
// rotated basis u=(ln wr0+ln wr1)/2 [memory length], v=(ln wr0-ln wr1)/2
// [hit/miss asymmetry], each with a dedicated updater class (Config_U /
// Config_V); relaxed wr box [0.00145, 0.3125]. Flat version: no globals,
// no tiers.  (Originally diagonal Newton only with the shared h00+h11
// curvature; the 2x2 (u,v) solve with the R_uv cross EMA of 050c below is
// now unconditional in Counter::C_Update.) RTRL traces stay in linear wr
// space.
// gcc 13.3 -O3 -march=haswell -ffast-math (linux, relative deltas only):
//   book1        345650 -> 344968 (-682,  -0.197%)
//   wcc386       313265 -> 311301 (-1964, -0.627%)
//   book1_wcc386 660884 -> 658352 (-2532, -0.383%)   total -5178 (-0.392%)
// Isolated contributions on top of baseline: LOGWR+UVROT alone (old box)
// -443; + relaxed box -4722 cum; + v/u step & clip re-tune -5178 cum.
// New C0_*_u / C0_*_v constants defined in sh_model-C0_h.inc style.
// -------------------------------------------------------------
// Both cell components adapt all four parameters (wr as the rotated u/v
// pair, mw and K) and both carry the A2 end-to-end gradient term; the
// ADAPT_WR/ADAPT_MW/ADAPT_K and C0_E2E_ON/S0_E2E_ON switches that used to
// make those optional are gone, along with the branches they selected.
// The rates themselves stay tunable: E2E = 0 in an .idx still turns the
// end-to-end blend off numerically.
// -------------------------------------------------------------
// Cache the post-step wr pair in the cell (+8 B) instead of recomputing the
// two expf at the next update: on for the order-1 cells (6 MB table), off
// for the SSE cells (1.6 GB, memory-bound).
#ifndef C0_CACHE_WR
#define C0_CACHE_WR 1
#endif
#ifndef S0_CACHE_WR
#define S0_CACHE_WR 0
#endif

// -------------------------------------------------------------
// Optimizer-proposal toggles (coder0_counter_scope.md / r8 doc).
// Every toggle defaults to 0 == bit-identical to the 050b-opt baseline.
// All changes live in Counter / ParamUpdater + new constants only.
// -------------------------------------------------------------
// 050c results (book1/wcc386/book1_wcc386 vs 050b 345333/310485/657402):
//   final: 344956/310144/656681 = -377/-341/-721; decode verified.
//   (Order-1 model alone, before the SSE and mixer stages existed; with both
//   stages bypassed the current coder gives 344899/309702, SSE-DESIGN.md
//   sec.5.  The pipeline numbers are in log.txt.)
// ON  (constants in sh_model-C0_h.inc retuned jointly for this stack):
//   OPT_MWLGT  mw in logit coords, own x-box mwXlo/mwXhi   -210 isolated
//   OPT_KLOG   K in ln coords, box = ln(kMin/kMax)          -96 isolated
//   OPT_UV2X2  2x2 u/v solve, R_uv EMA cross (+1 float/ctr) -74 isolated
//   OPT_DIAMP  diamond proj of (u,v) step                   ~-17 on stack only
//   joint retune of NWm/M1m/M2m/mwStep/NWk/kStep/M1k/M2k/CXW/XHW: rest.
//   EFISH_m (-88 on linear mw) is subsumed by MWLGT: EFW_m -> 0 on stack.
// OFF (tested, no win): OPT_G1 (+366..), OPT_AWIND (0 on stack), OPT_AWOSC
//   (+3768), OPT_HBETA (file-antagonistic), OPT_EFISH (see above), OPT_ALEAK
//   (book1-only, concat +), OPT_CSHRK (=UV2X2 at weak coupling), OPT_SMASS
//   (+1349, stale mass at file boundary), OPT_BIASC (+43..+294 monotone),
//   OPT_MKCPL (all 4 variants: realized/EFISH cross x RAYCL/per-axis, both
//   signs; best dose CMK=16/EFISH = +6 noise, everything else worse; the
//   g_m*g_k channel is empty -- gradients already share dpK_dpmix/stP
//   structure. Side finding: joint RAYCL on (mw,K) costs +131 by itself
//   [OPT_MKRAY], mw/K saturate too often for a shared trust region).
// d2p/ww note: the u/v d2p channel is only dead while G2_u = G2_v = 0 and
//   XHW*XHC = 0.  In the tuned constants C0_G2_v = 4 (clip 0.125) and
//   S0_G2_u = 1 (clip 0.031), so the channel is live through h_d, and in C0
//   the n?_ww? traces also feed the cross term h_x (XHC = 100).  Deleting the
//   ww traces would need all of those set to 0, tested as one toggle.
// Baseline recovery: -DOPT_MWLGT=0 -DOPT_KLOG=0 -DOPT_UV2X2=0 -DOPT_DIAMP=0
//   plus 050b values of the 16 retuned constants (kept in .inc comments).
// -------------------------------------------------------------
// (OPT_UV2X2 is the only variant carried: the 2x2 (u,v) solve is unconditional in
// Counter::C_Update; OPT_MKCPL was dropped with the template refactor.)
// -------------------------------------------------------------
// 061 (2026-09), from coder0_optimizer_improvements_v3.md; measurements in
// SSE-DESIGN.md sec.6.11 (reference 235391/275195/512847 book1/wcc386/book1wcc):
//   A1  per-cell update count (Counter: in the former pK slot, the caller
//       passes pK to C_Update; Mix2: +4 B) and the young-cell step limit
//       stepMax*(1 + AGA/(1 + age/B)), knobs *_AGAu/AGAm/AGAk/AGB, M0_AGAw/AGAb/AGB.
//       S0 (mw, K, u/v ray) -472, M0 bias -498; C0 and the M0 weight: no.
//   A2  end-to-end gradients: C_Update(bit, pK, g, ef) blends the cell's own
//       error with the final coder's, chained through the mixer weight and
//       the SSE interpolation (main() computes the chain).  C0_E2E = 1 -2161
//       (with C0 rates x1.5 -2534), S0_E2E = 0.25 -137.  The stack -3781.
//   A3  hypergradient on the stage rates: loses at every setting, removed.
//   F4  NW after the 2x2 solve: +112 alone, -63 vs. the stack; off.
//   F13 [[no_unique_address]] on the members that can be empty.
//   The compile-time diagnostics E2E_NOCHAIN (chain factor = the mixer weight
//   alone), E2E_SSEI (SSE cells train on the interpolated output's own loss)
//   and UV_NW_AFTER (F4) all measured worse and have been removed, as was
//   E2E_HESS, the end-to-end curvature.
//   Divisions/transcendentals per bit cut from ~62/36 to ~33/24 (SSE-DESIGN
//   sec.6.12): PredictF hands its intermediates to C_Update, the stages
//   pass logits, the gradient chain runs in the logit domain.
//   Schraudolph's bit-trick exp/log/stretch (schrau.inc) in place of libm's:
//   -16% time, +0.014% size (sec.6.14).  Unconditional -- the knobs in IDX/
//   are tuned for it, so the libm variant is no longer a build option.
//   Tuned (optv.pl): 234213/273291/509525 = -0.63% vs. the 061 baseline.
//   Build identity (F5): rt_logf/rt_expf now carry a volatile barrier (they
//   were inlined and folded under LTO), and the RTRL leaks are clamped to
//   [0,1] (a leak above 1 overflows the traces, and clip() of inf differs
//   between the builds).  -DTRACE_P dumps the per-bit probabilities.

// C library headers
#include <stdlib.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>

// Intel SSE/AVX intrinsics to fix the denormal microcode penalty
#if defined(__SSE3__) || defined(__x86_64__) || defined(_M_X64)
#include <x86intrin.h>
#endif

typedef unsigned short word;
typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned long long qword;

// [[no_unique_address]] for the one Counter member that can be empty,
// WrCache<0> (the SSE cells do not cache the post-step wr pair): without it
// it costs 1 byte plus padding, taking the 96 B cell to 104 B for nothing
// (SSE-DESIGN.md sec.3.6).  MSVC (and clang in MS mode) ignore the standard
// spelling and need the msvc:: one.
// gradtest.cpp (gt.sh) builds the coder with -DGRAD_TEST and checks every
// gradient/curvature term handed to ParamUpdater against finite
// differences; the hook records them and is empty otherwise.
#ifdef GRAD_TEST
 void grad_test_hook( float gd, float gr, float hc );
 #define GRAD_HOOK(gd,gr,hc) grad_test_hook(gd,gr,hc)
#else
 #define GRAD_HOOK(gd,gr,hc) ((void)0)
#endif

#ifdef _MSC_VER
 #define NUA [[msvc::no_unique_address]]
#else
 #define NUA [[no_unique_address]]
#endif

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

uint flen( FILE* f ) {
  fseek( f, 0, SEEK_END );
  uint len = ftell(f);
  fseek( f, 0, SEEK_SET );
  return len;
}

#include "sh_v2f.inc"
static const int mSCALE = SCALE-1;
static const float iSCALE = 1.0f/SCALE;

#include "sh_mapping.inc"
#include "MOD/sh_model-C0_h.inc"
// S0/M0 knobs, context masks and context builders (their Table() storage
// comes with the _h.inc headers below, once the cell types are complete)
#include "MOD/sh_model-S0_p.inc"
#include "MOD/sh_model-M0_p.inc"

// Table() element counts of the tuning build (IDX-FORMAT.md sec.9)
static inline unsigned long long tbl_n( unsigned long long n ) { return n; }



// ---------------------------------------------


static inline float clip(float x, float d) {
  return fminf(fmaxf(x, -d), d);
}

static inline float clamp(float p) {
  return fminf(fmaxf(p, 1.0f), float(mSCALE));
}

static inline float clamp(float x, float min_val, float max_val) {
  return fminf(fmaxf(x, min_val), max_val);
}

// Schraudolph's approximations replace logf/expf from here on, the init-time
// wrappers below included (schrau.inc).
#include "schrau.inc"

// Init-time transcendentals that must not be constant-folded: the shipping
// build would fold a log/exp of a folded knob at compile time while the
// tuning build evaluates it at runtime, and the two can differ by an ulp --
// enough to change the stream by a byte.  The volatile copy is the barrier:
// a plain static wrapper is inlined under -O3/LTO and folded anyway (seen on
// KYHI = ln(kMax) at kMax = 1.0311, where libm's logf is an ulp below MPFR's:
// the K box wall differed and the streams drifted by a byte).  gc.sh also
// passes -fno-builtin-logf -fno-builtin-expf, which closes the same hole for
// any libm logf/expf of a constant elsewhere.
static float rt_logf( float x ) { volatile float v = x; return logf(v); }
static float rt_expf( float x ) { volatile float v = x; return expf(v); }


// --- Configuration Struct Declarations ---

#define def_Config(Config) struct Config {\
  static const float momentum_D, momentum_R, NW, inc, stepMax, minVal,maxVal, grad2_clip, D_clip, R_clip, R0; \
  static const int NAG; };

// Parameter bundle of the order-1 model: C0_* constants from
// IDX/sh_model-C0.idx.
#define CP_NAME     CP_C0
#define CP_PFX      C0_
#define CP_CACHE_WR C0_CACHE_WR
#include "config.hpp"

// Parameter bundle of the SSE cells: S0_* constants from IDX/sh_model-S0.idx.
#define CP_NAME     CP_S0
#define CP_PFX      S0_
#define CP_CACHE_WR S0_CACHE_WR
#define CP_SSE      1
#include "config.hpp"

// Parameter bundle of the 2-input mixer: M0_* constants from IDX/sh_model-M0.idx.
#define CP_NAME     CP_M0
#define CP_PFX      M0_
#include "config_mix2.hpp"


#include "sh_pupdater.inc"

#include "sh_counter.inc"
#include "sh_SSE.inc"
#include "sh_mix2.inc"

// Cell types and table sizes of the two stages, then the generated structs
// that hold their Table() storage: S0_T::S0_tbl / M0_T::M0_tbl are fixed
// arrays in the shipping build and pointers allocated by S0_Init()/M0_Init()
// in the tuning build.  The size helpers are constant expressions in the
// shipping build (they size the arrays); in the tuning build the knobs they
// read are runtime values, so there they are plain functions.
#if defined(USE_NEW) && USE_NEW
#define TBL_CONSTEXPR
#else
#define TBL_CONSTEXPR constexpr
#endif
typedef Counter<CP_S0> SSE_Cell;
static TBL_CONSTEXPR qword sse_table_cells( qword volume ) {
  return sse_rows( volume, CP_S0::HBITS, sse_nb_clamp(CP_S0::NB) ) * qword(sse_nb_clamp(CP_S0::NB));
}
typedef Mix2<CP_M0>::Cell Mix2_Cell;
static TBL_CONSTEXPR uint mix_table_ctx( qword volume ) {
  return mix_rows( volume );
}

#include "MOD/sh_model-S0_h.inc"
#include "MOD/sh_model-M0_h.inc"

static const uint CNUM = 256;

ALIGN(64) Rangecoder rc;
Counter<CP_C0> o1[256][256];

// USE_NEW comes from the generated IDX headers: 1 = tuning build (knobs are
// runtime values, dispatch on NB), 0 = shipping build (NB folded).
S0_T S0;           // SSE cell table
M0_T M0;           // mixer cell table
#if USE_NEW
SSE_Dyn<CP_S0> sse;
#else
SSE_Ctr<CP_S0, sse_nb_clamp(CP_S0::NB)> sse;
#endif
Mix2<CP_M0> mix;   // final p = mix( order-1 prediction, SSE output )

int main( int argc, char** argv ) {
  uint f_DEC, i, j, c=0, f_len, f_pos, cxt, bit=0, p;
  FILE* f;
  FILE* g;

#if defined(__SSE3__) || defined(__x86_64__) || defined(_M_X64)
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

  if( argc < 4 ) {
    print_usage:
    printf(
      "O1 Compressor - Order-1 adaptive lossless compression + SSE stage\n"
      "\n"
      "Usage: %s <mode> <input> <output>\n"
      "\n"
      "Arguments:\n"
      "  <mode>    'c' for compress, 'd' for decompress\n"
      "  <input>   Input file path\n"
      "  <output>  Output file path\n"
      "\n", argv[0]
    );
    return 1;
  }

  f_DEC = (argv[1][0]=='d');
  f = fopen(argv[2],"rb"); if( f==0 ) return 2;
  g = fopen(argv[3],"wb"); if( g==0 ) return 3;

  if( f_DEC==0 ) {
    f_len = flen(f);
    fwrite( &f_len, 1,sizeof(f_len), g );
    rc.StartEncode(g);
  } else {
    f_len = 0;
    if( fread( &f_len, 1,sizeof(f_len), f ) != sizeof(f_len) ) { fclose(g); fclose(f); return 4; }
    rc.StartDecode(f);
  }

  // Initialize Order-1 Predictor array
  for( i=0; i<CNUM; i++) for( j=0; j<CNUM; j++ ) o1[i][j].Init();
  S0.S0_Init(); M0.M0_Init();   // tuning build: allocate the tables
  sse.Init( S0.S0_tbl, qword(S0_Cx_Volume)*S0_Cx3_Volume );
  mix.Init( M0.M0_tbl, M0_Cx_Volume );

  int last_c = 0, c2 = 0, c3 = 0;

  for( f_pos=0; f_pos<f_len; f_pos++ ) {
    if( f_DEC==0 ) c = getc(f);

    for( cxt=1; cxt<CNUM; ) {
      if( f_DEC==0 ) bit=(c>>7)&1;

      // p = mix( order-1 prediction, SSE(order-1 prediction) )
      // The stages pass logits: the SSE quantizes the order-1 logit z1 and
      // returns its interpolated logit z2, the mixer blends the two in the
      // stretch domain -- the same values as st(p1), st(p2) up to rounding,
      // without the five logf and the sq that produced and consumed p2.
      Counter<CP_C0>::Pred pr = o1[last_c][cxt].PredictF();
      float z1 = pr.z;
      qword cx = qword(S0_MakeCx(c2, last_c, cxt))*S0_Cx3_Volume + S0_MakeCx3(c3);
      float z2 = sse.Predict( cx, z1 );
      float pf = mix.Mix( M0_MakeCx(c2, last_c, cxt), z1, z2 );
      p = uint( clamp( pf*float(SCALE) ) );
#ifdef TRACE_P
      // -DTRACE_P: per-bit trace of (p1, p2, pf, p) as floats to $TRACE_P, to
      // find the first divergent bit between two builds (SSE-DESIGN.md sec.4.1)
      { static FILE* trf = fopen( getenv("TRACE_P") ? getenv("TRACE_P") : "trace.bin", "wb" );
        float v[4] = { pr.pK, z2, pf, float(p) }; if( trf ) fwrite( v, 4, 4, trf ); }
#endif
      
      bit = rc.rc_BProcess( p, bit );

      // A2: e_f = p_f - [bit==0] = dL/dz_f; chain to the SSE output and the
      // order-1 logit (the SSE input also moves the interpolation point).
      // dzf_dz1/dzf_dz2 are the mixer weights, 0 where its input clip binds.
      float e_f = pf - float(1 - bit);
      float c_2 = mix.dzf_dz2();
      float c_1 = mix.dzf_dz1() + c_2 * sse.dz2_dz1();
      o1[last_c][cxt].C_Update( bit, pr, 1.0f, e_f * c_1 );
      sse.Update( bit, e_f * c_2 );
      mix.Update( bit );

      c<<=1; cxt+=cxt+bit;
    }

    cxt = byte(cxt);

    if( f_DEC==1 ) putc(cxt,g);

    c3 = c2; c2 = last_c; last_c = cxt; 
  }

  if( f_DEC==0 ) rc.FinishEncode();
  S0.S0_Quit(); M0.M0_Quit();

  fclose(g);
  fclose(f);

  return 0;
}
