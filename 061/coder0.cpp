
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
// Also fixed: trailing RTRL leakage lines now #if ADAPT_WR guarded
// (ADAPT_WR=0 didn't compile before).
// -------------------------------------------------------------
// Toggles for enabling/disabling parameter adaptations.
// Set to 0 to disable an update entirely (saves memory & compute).
// -------------------------------------------------------------
#define ADAPT_WR 1
#define ADAPT_MW 1
#define ADAPT_K  1
// Same toggles for the SSE cells (sh_SSE2.inc, parameters S0_*); these and
// SSE_MAXCELLS_LOG may be overridden from the build line (gc.sh CXXEXTRA).
#ifndef S0_ADAPT_WR
#define S0_ADAPT_WR 1
#endif
#ifndef S0_ADAPT_MW
#define S0_ADAPT_MW 1
#endif
#ifndef S0_ADAPT_K
#define S0_ADAPT_K  1
#endif
// A2 end-to-end gradient path (C0_E2E / S0_E2E knobs): compiled in by
// default, since E2E = 0 is bit-identical; 0 removes the per-bit chain.
#ifndef C0_E2E_ON
#define C0_E2E_ON 1
#endif
#ifndef S0_E2E_ON
#define S0_E2E_ON 1
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

// [[no_unique_address]] for the Counter members that are empty when an
// adaptation is off (ParamUpdater<0,..>, RTRLState<0>): without it each costs
// 1 byte plus padding, and the reduced cells are 44/20 B instead of the
// intended 36/12 B (SSE-DESIGN.md sec.3.6).  MSVC (and clang in MS mode)
// ignore the standard spelling and need the msvc:: one.
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
//#include "sh_model-C0_h.inc"
// S0/M0 knobs, context masks and context builders (their Table() storage
// comes with the _h.inc headers below, once the cell types are complete)
#include "MOD/sh_model-S0_p.inc"
#include "MOD/sh_model-M0_p.inc"

// Table() element counts of the tuning build (IDX-FORMAT.md sec.9)
static inline unsigned long long tbl_n( unsigned long long n ) { return n; }



// ---------------------------------------------

static inline float Max( float x, float d ) {
  return fmax(d,x);
}

static inline float clip(float x, float d) {
  return fminf(fmaxf(x, -d), d);
}

static inline float clamp(float p) {
  return fminf(fmaxf(p, 1.0f), float(mSCALE));
}

static inline float clamp(float x, float min_val, float max_val) {
  return fminf(fmaxf(x, min_val), max_val);
}

// Init-time transcendentals that must not be constant-folded: the shipping
// build would fold logf/expf of folded knobs at compile time (MPFR,
// correctly rounded) while the tuning build calls libm at runtime, and the
// two can differ by an ulp -- enough to change the stream by a byte.  Both
// builds go through libm for these; the per-bit paths are runtime anyway.
static float rt_logf( float x ) { return logf(x); }
static float rt_expf( float x ) { return expf(x); }

// --- Configuration Struct Declarations ---

#define def_Config(Config) struct Config {\
  static const float momentum_D, momentum_R, NW, inc, stepMax, minVal,maxVal, grad1_clip, grad2_clip, D_clip, R_clip, R0, hbeta, efw; \
  static const int NAG; };

// Parameter bundle of the order-1 model: C0_* constants from
// IDX/sh_model-C0.idx, adaptation flags from the ADAPT_* toggles above.
#define CP_NAME     CP_C0
#define CP_PFX      C0_
#define CP_ADAPT_WR ADAPT_WR
#define CP_ADAPT_MW ADAPT_MW
#define CP_ADAPT_K  ADAPT_K
#define CP_E2E      C0_E2E_ON
#include "config.hpp"

// Parameter bundle of the SSE cells: S0_* constants from IDX/sh_model-S0.idx.
#define CP_NAME     CP_S0
#define CP_PFX      S0_
#define CP_ADAPT_WR S0_ADAPT_WR
#define CP_ADAPT_MW S0_ADAPT_MW
#define CP_ADAPT_K  S0_ADAPT_K
#define CP_E2E      S0_E2E_ON
#define CP_SSE      1
#include "config.hpp"

// Parameter bundle of the 2-input mixer: M0_* constants from IDX/sh_model-M0.idx.
#define CP_NAME     CP_M0
#define CP_PFX      M0_
#include "mix2cfg.hpp"


template<int ADAPT, class cfg> struct ParamUpdater;

// Active parameter state 
// Split into Accum (D/R statistics) / StepRaw (Newton step) / Apply (clip,
// move, box) so the UV2X2 coupled solve can interpose between the halves;
// Update() composes them and is bit-identical to the pre-split code.
template<class cfg> struct ParamUpdater<1, cfg> {
  float val;
  float D;
  float R;

  void Init(float init_val) {
    val = init_val;
    D = 0.0f;
    R = cfg::R0;
  }

  // g = weight of this observation (1 = a full event); scales the loss.
  // gs scales the gradient alone (A2: the end-to-end error replaces the
  // cell's own; 1 = the cell's own loss, bit-identical).  The curvature R
  // stays the cell's own, as the preconditioner.
  // Returns the gradient term dp_inv (for the Nesterov look-ahead).
  INLINE float Accum(
    float dp, float d2p, float inv_p, float inv_p2, float inv_pq, float g = 1.0f, float gs = 1.0f
  ) {
    float dp_inv = dp * inv_p * g * gs;
    float dp2_inv = (dp * dp) * inv_p2 * g;
    (void)inv_pq;
    float d2p_inv = d2p * inv_p * g;

    // --- GRADIENT CLIPPING ---
    d2p_inv= clip( d2p_inv, cfg::grad2_clip );
    // -------------------------

    D = D * cfg::momentum_D - dp_inv;
    R = R * cfg::momentum_R + dp2_inv - d2p_inv;

    D = clip(D,cfg::D_clip);
    //R = clip(R,cfg::R_clip);
    R = clamp(R,0.0f,cfg::R_clip);
    return dp_inv;
  }

  INLINE float Denom() const { return R + cfg::inc; }

  INLINE float StepRaw() const {
    //float safe_R = fabsf(R) + cfg::inc;
    //float safe_R = Max(R,0.0f) + cfg::inc;
    float safe_R = R + cfg::inc;
    return cfg::NW * D / safe_R;
  }

  // smax: the step limit, cfg::stepMax unless the caller schedules it (A1)
  INLINE void Apply( float step, float lo, float hi, float smax = cfg::stepMax ) {
    step = clip(step, smax);

    float nv = val - step;
    val = clamp(nv, lo, hi);
  }

  void Update(
    float dp, float d2p, float inv_p, float inv_p2, float inv_pq, float sc,
    float lo, float hi, float g = 1.0f, float smax = cfg::stepMax, float gs = 1.0f
  ) {
    float dp_inv = Accum(dp, d2p, inv_p, inv_p2, inv_pq, g, gs);
    (void)sc;
    if( cfg::NAG ) {
      // Nesterov look-ahead: the step from momentum_D*D_new - g instead of D_new
      float Dn = D * cfg::momentum_D - dp_inv;
      Apply( cfg::NW * Dn / (R + cfg::inc), lo, hi, smax );
    } else Apply(StepRaw(), lo, hi, smax);
  }

};

// Inactive parameter state
template<class cfg> struct ParamUpdater<0, cfg> {
  void Init(float /*init_val*/) {}
  inline float Accum(float, float, float, float, float, float = 1.0f, float = 1.0f) { return 0.0f; }
  inline float Denom() const { return 1.0f; }
  inline float StepRaw() const { return 0.0f; }
  inline void Apply(float, float, float, float = 0.0f) {}
  inline void Update(float /*dp*/, float /*d2p*/, float /*inv_p*/, float /*inv_p2*/, float /*inv_pq*/, float /*sc*/, float /*lo*/, float /*hi*/, float = 1.0f, float = 0.0f, float = 1.0f) {}
};


// Real-Time Recurrent Learning traces of {n0, n1} w.r.t. wr0/wr1, plus the
// UV2X2 cross-curvature EMA.  Split out of Counter so that a component with
// ADAPT_WR=0 (e.g. SSE cells) carries none of these 9 floats.
template<int ADAPT> struct RTRLState {
  float n0_w0, n0_ww0, n0_w1, n0_ww1;
  float n1_w0, n1_ww0, n1_w1, n1_ww1;
  float R_uv;   // 2.1: EMA of the rotated (u,v) cross curvature (+1 float)
};
template<> struct RTRLState<0> {};


// Counter<CP>: one adaptive binary probability cell.  CP is a parameter
// bundle made by config.hpp (CP_C0 for the order-1 model, CP_S0 for the SSE
// cells): it carries the ADAPT_* flags, the four ParamUpdater configs and
// every derived constant the counter reads.  The arithmetic is exactly the
// pre-template Counter's; the #if ADAPT_* blocks became if constexpr.
template<class CP> struct Counter {
  typedef typename CP::Config_U  Config_U;
  typedef typename CP::Config_V  Config_V;
  typedef typename CP::Config_MW Config_MW;
  typedef typename CP::Config_K  Config_K;

  float n0;
  float n1;
  // Updates this cell has seen (saturating).  Drives the young-cell step
  // schedule (A1): stepMax_eff = stepMax*(1 + AGA/(1 + age/B)) on the mw/K
  // steps and on the u/v ray clip, so a fresh cell may move faster than a
  // settled one.  It lives in what used to be the pK slot: the cell's own
  // prediction is now passed back into C_Update() by the caller (which has
  // it from PredictF() anyway), so the cell stays 96 B.
  uint  age;


  // Context-adaptive parameters logic encapsulating both wr limits
  // UVROT: wr0_state tracks u = (ln wr0 + ln wr1)/2,
  //        wr1_state tracks v = (ln wr0 - ln wr1)/2
  NUA ParamUpdater<CP::A_WR, Config_U>  wr0_state;
  NUA ParamUpdater<CP::A_WR, Config_V>  wr1_state;
  NUA ParamUpdater<CP::A_MW, Config_MW> mw_state;
  NUA ParamUpdater<CP::A_K,  Config_K>  k_state;

  // Real-Time Recurrent Learning helper states for {n0, n1} variables
  NUA RTRLState<CP::A_WR> rt;

  static float st( const float p_ ) {
    float p = p_ * 0.999998f + 0.000001f;
    return logf(p/(1.0f-p));
  }

  static float sq( const float x ) {
    return 1.0f/(1.0f+expf(-x));
  }

  // Init with explicit counts; SSE cells start at their bucket's probability.
  void InitN( float a, float b ) {
    n0 = a;
    n1 = b;
    if( n0+n1==0.0f ) {
      n0 = iSCALE; 
      n1 = iSCALE;
    }
    age = 0;


    wr0_state.Init(CP::UV_U0);
    wr1_state.Init(CP::UV_V0);
    mw_state.Init(CP::MWX0);
    k_state.Init(CP::KY0);

    if constexpr( CP::A_WR ) {
      rt.n0_w0 = 0.0f; rt.n0_ww0 = 0.0f; rt.n0_w1 = 0.0f; rt.n0_ww1 = 0.0f;
      rt.n1_w0 = 0.0f; rt.n1_ww0 = 0.0f; rt.n1_w1 = 0.0f; rt.n1_ww1 = 0.0f;
      rt.R_uv = 0.0f;
    }
  } 

  void Init() {
    InitN( CP::F0_P0, CP::F0_P1 );
  }

  // Current mixing weight / logistic scale (adaptive or the fixed seed)
  float cur_mw() const {
    if constexpr( CP::A_MW ) return Config_MW::minVal + CP::MWspan * sq(mw_state.val);
    else                         return CP::M;
  }
  float cur_K() const {
    if constexpr( CP::A_K ) return expf(k_state.val);
    else                        return CP::K;
  }

  // Raw probability of bit==0; the caller keeps it for C_Update()
  float PredictF() const {
    float cur_mw = this->cur_mw();
    float curr_K = this->cur_K();

    float n_sum = n0 + n1 + 1e-8f;
    float p0 = n0 / n_sum;
    p0 = p0 * (1.0f - cur_mw) + CP::mwP0 * cur_mw;
    return sq(curr_K * st(p0));
  }

  float Predict() const {
    float p_out = PredictF() * float(SCALE);
    p_out = clamp(p_out);
    return p_out;
  }

  // pK = this cell's PredictF() for the bit being coded.
  // g = weight of this observation, 1 = a full event (bit-identical to the
  // unweighted update).  A fractional event scales the loss by g, and in the
  // count recursion every wr becomes wr*g and every injected count g, so
  // the RTRL traces stay the exact derivatives of the weighted recursion.
  // ef = A2 end-to-end error: dL_final/dz where z = st(pK) is this cell's
  // logit output, i.e. (p_final - [bit==0]) * dz_final/dz.  The cell's own
  // error is e = pK - [bit==0]; the gradient uses (1-E2E)*e + E2E*ef, the
  // curvature stays the cell's own.  E2E = 0 ignores ef (bit-identical).
  void C_Update( const int bit, const float pK, const float g = 1.0f, const float ef = 0.0f ) {
    float sign = 1.0f - float(bit + bit); 

    // A1: young-cell step schedule, per axis: stepMax*(1 + AGA*agd),
    // agd = 1/(1 + age/B).  AGA = 0 reproduces the plain limit bit for bit.
    float agd = 1.0f / (1.0f + float(age) * CP::AGiB);
    if( age != ~0u ) age++;

    float pK_t = pK * sign + float(bit);
    if( pK_t < 1e-12f ) pK_t = 1e-12f;
    float inv_pK_t = 1.0f / pK_t;
    float inv_pK_t2 = inv_pK_t * inv_pK_t;

    // A2: gradient scale gs = ((1-E2E)*e + E2E*ef) / e with e = pK-[bit==0]
    // = -sign*p_o, p_o = probability of the other symbol; 1 when E2E = 0.
    float gs = 1.0f;
    if constexpr( CP::E2E_ON ) {
      float p_o = 1.0f - pK_t; if( p_o < 1e-12f ) p_o = 1e-12f;
      gs = (1.0f - CP::E2E) - CP::E2E * ef * sign / p_o;
    }

    const float inv_pq = 0.0f;
    const float bias_sc = 1.0f;

    float cur_wr0, cur_wr1;
    if constexpr( CP::A_WR ) {
      cur_wr0 = expf(clamp(wr0_state.val + wr1_state.val, CP::UVLO, CP::UVHI));
      cur_wr1 = expf(clamp(wr0_state.val - wr1_state.val, CP::UVLO, CP::UVHI));
    } else {
      cur_wr0 = CP::W0;
      cur_wr1 = CP::W1;
    }

    float mws = 0.0f, cur_mw;
    if constexpr( CP::A_MW ) {
      mws = sq(mw_state.val);
      cur_mw = Config_MW::minVal + CP::MWspan * mws;
    } else {
      cur_mw = CP::M;
    }

    float curr_K = this->cur_K();

    float n_sum = n0 + n1 + 1e-8f;
    float q0 = n0 / n_sum;
    float p_mix = q0 * (1.0f - cur_mw) + CP::mwP0 * cur_mw;

    float P_adj = p_mix * 0.999998f + 0.000001f;
    float P_adj_inv = 1.0f / P_adj;
    float P_adj_inv_comp = 1.0f / (1.0f - P_adj);
    float d_st_dP = 0.999998f * (P_adj_inv + P_adj_inv_comp);
    float d2_st_dP2 = (0.999998f * 0.999998f) * (P_adj_inv_comp * P_adj_inv_comp - P_adj_inv * P_adj_inv);

    float dpK_dpmix = pK * (1.0f - pK) * curr_K * d_st_dP;
    float d2pK_dpmix2 = (1.0f - 2.0f * pK) * dpK_dpmix * curr_K * d_st_dP + pK * (1.0f - pK) * curr_K * d2_st_dP2;

    // --- 1. Update WR0 and WR1 parameters based on current derivatives ---
    if constexpr( CP::A_WR ) {
    float dpmix_dq0 = (1.0f - cur_mw);
    float dpK_dq0 = dpK_dpmix * dpmix_dq0;
    float d2pK_dq02 = d2pK_dpmix2 * (dpmix_dq0 * dpmix_dq0);

    float inv_n_sum = 1.0f / n_sum;
    float inv_n_sum2 = inv_n_sum * inv_n_sum;
    float inv_n_sum3 = inv_n_sum2 * inv_n_sum;

    float dq0_dn0 = n1 * inv_n_sum2;
    float dq0_dn1 = -n0 * inv_n_sum2;

    float d2q0_dn02 = -2.0f * n1 * inv_n_sum3;
    float d2q0_dn12 =  2.0f * n0 * inv_n_sum3;
    float cross = (n0 - n1) * inv_n_sum3;

    // Chain to parameters
    float dq0_dwr0 = dq0_dn0 * rt.n0_w0 + dq0_dn1 * rt.n1_w0;
    float dq0_dwr1 = dq0_dn0 * rt.n0_w1 + dq0_dn1 * rt.n1_w1;

    float d2q0_dwr02 = d2q0_dn02 * (rt.n0_w0*rt.n0_w0) + d2q0_dn12 * (rt.n1_w0*rt.n1_w0) + 2.0f * cross * rt.n0_w0 * rt.n1_w0 
                     + dq0_dn0 * rt.n0_ww0 + dq0_dn1 * rt.n1_ww0;

    float d2q0_dwr12 = d2q0_dn02 * (rt.n0_w1*rt.n0_w1) + d2q0_dn12 * (rt.n1_w1*rt.n1_w1) + 2.0f * cross * rt.n0_w1 * rt.n1_w1
                     + dq0_dn0 * rt.n0_ww1 + dq0_dn1 * rt.n1_ww1;

    float dpK_dwr0 = dpK_dq0 * dq0_dwr0;
    float d2pK_dwr02 = d2pK_dq02 * (dq0_dwr0 * dq0_dwr0) + dpK_dq0 * d2q0_dwr02;

    float dpK_dwr1 = dpK_dq0 * dq0_dwr1;
    float d2pK_dwr12 = d2pK_dq02 * (dq0_dwr1 * dq0_dwr1) + dpK_dq0 * d2q0_dwr12;

    // LOGWR: chain to log coordinates, u? = ln(wr?):
    // dp/du = dp/dwr * wr ; d2p/du2 = d2p/dwr2 * wr^2 + dp/dwr * wr
    d2pK_dwr02 = d2pK_dwr02*(cur_wr0*cur_wr0) + dpK_dwr0*cur_wr0;
    d2pK_dwr12 = d2pK_dwr12*(cur_wr1*cur_wr1) + dpK_dwr1*cur_wr1;
    dpK_dwr0 *= cur_wr0;
    dpK_dwr1 *= cur_wr1;

    // UVROT: rotate to u = (u0+u1)/2, v = (u0-u1)/2; diagonal Newton with
    // the shared curvature approximation h_uu ~ h_vv ~ h00 + h11.
    float g_u = dpK_dwr0 + dpK_dwr1;
    float g_v = dpK_dwr0 - dpK_dwr1;
    float h_d = d2pK_dwr02 + d2pK_dwr12;


    // 2.1 UV2X2 + XHESS + RAYCL: one EMA cross term, 2x2 Newton solve with
    // det guard (or P11 CSHRK soft shrink), joint direction-preserving
    // trust region. Reduces to the diagonal path when the guard trips.
    wr0_state.Accum(g_u * sign, h_d * sign, inv_pK_t, inv_pK_t2, inv_pq, g, gs);
    wr1_state.Accum(g_v * sign, h_d * sign, inv_pK_t, inv_pK_t2, inv_pq, g, gs);

    // XHESS: exact rotated cross diagonal h_uv = h00 - h11 (MIXTR-free part),
    // exposed through the signed shaper XHW with its own clip (G2_u/v are 0
    // in the delivered constants, so the cross channel gets a dedicated one).
    float h_x = clip( (d2pK_dwr02 - d2pK_dwr12) * sign * inv_pK_t, CP::XHC );
    rt.R_uv = rt.R_uv * Config_U::momentum_R + CP::CXW * (g_u * g_v * inv_pK_t2 - CP::XHW * h_x) * g;

    {
      float a_u = wr0_state.Denom();
      float a_v = wr1_state.Denom();
      float cc  = rt.R_uv;
      float det = a_u * a_v - cc * cc;
      if( det > CP::UVDET * a_u * a_v ) {
        float b_u = Config_U::NW * wr0_state.D;
        float b_v = Config_V::NW * wr1_state.D;
        float idet = 1.0f / det;
        float s_u = (b_u * a_v - cc * b_v) * idet;
        float s_v = (a_u * b_v - cc * b_u) * idet;
        // RAYCL: rescale jointly so neither component exceeds its stepMax
        // (widened by the age schedule while the cell is young).
        float agu = 1.0f + CP::AGAu * agd;
        float r = fmaxf( fmaxf( fabsf(s_u)*CP::iStepU, fabsf(s_v)*CP::iStepV ) / agu, 1.0f );
        float ir = 1.0f / r;
        wr0_state.Apply(s_u * ir, CP::UVLO, CP::UVHI, Config_U::stepMax * agu);
        wr1_state.Apply(s_v * ir, -CP::UV_VH, CP::UV_VH, Config_V::stepMax * agu);
      } else {
        float agu = 1.0f + CP::AGAu * agd;
        wr0_state.Apply(wr0_state.StepRaw(), CP::UVLO, CP::UVHI, Config_U::stepMax * agu);
        wr1_state.Apply(wr1_state.StepRaw(), -CP::UV_VH, CP::UV_VH, Config_V::stepMax * agu);
      }
    }

    // 2.2 DIAMP: replace the two independent boxes by the clamp in original
    // (ln wr0, ln wr1) coordinates + back-rotation; kills the corner where
    // u,v are individually legal but u+-v is pinned at the exp-time clamp
    // (measured 26-37% of wr updates) while the stored coordinates drift.
    {
      float t0 = clamp(wr0_state.val + wr1_state.val, CP::UVLO, CP::UVHI);
      float t1 = clamp(wr0_state.val - wr1_state.val, CP::UVLO, CP::UVHI);
      float nu = 0.5f * (t0 + t1);
      float nv = 0.5f * (t0 - t1);
      wr0_state.val = nu;
      wr1_state.val = nv;
    }
    }  // ADAPT_WR

    // (The P10 MKCPL coupled (mw,K) variant that used to sit here was dead
    // code -- OPT_MKCPL=0, tested as a loss, see the header notes -- and is
    // not carried into the template.)

    // --- 2. Update MW parameter ---
    if constexpr( CP::A_MW ) {
    float dpmix_dmw = CP::mwP0 - q0;
    float dpK_dmw = dpK_dpmix * dpmix_dmw;
    float d2pK_dmw2 = d2pK_dpmix2 * (dpmix_dmw * dpmix_dmw);

    // P24: chain to the logit coordinate, mw = lo + span*sigma(x):
    // f' = span*s(1-s), f'' = f'*(1-2s) (the commonly-dropped term kept).
    {
      float f1m = CP::MWspan * mws * (1.0f - mws);
      float f2m = f1m * (1.0f - 2.0f * mws);
      float g_xm = dpK_dmw * f1m;
      float h_xm = d2pK_dmw2 * (f1m * f1m) + dpK_dmw * f2m;
      mw_state.Update(g_xm * sign, h_xm * sign, inv_pK_t, inv_pK_t2, inv_pq, bias_sc, CP::MWXLO, CP::MWXHI, g,
                      Config_MW::stepMax * (1.0f + CP::AGAm * agd), gs);
    }
    }  // ADAPT_MW

    // --- 3. Update K parameter ---
    if constexpr( CP::A_K ) {
    float stP = st(p_mix);
    float dpK_dK = pK * (1.0f - pK) * stP;
    float d2pK_dK2 = dpK_dK * (1.0f - 2.0f * pK) * stP;
    
    if( fabsf(stP)>=CP::stP_min ) {
      // P24: chain to y = ln K: f' = f'' = K.
      float g_yk = dpK_dK * curr_K;
      float h_yk = d2pK_dK2 * (curr_K * curr_K) + dpK_dK * curr_K;
      k_state.Update(g_yk * sign, h_yk * sign, inv_pK_t, inv_pK_t2, inv_pq, bias_sc, CP::KYLO, CP::KYHI, g,
                     Config_K::stepMax * (1.0f + CP::AGAk * agd), gs);
    }
    }  // ADAPT_K


    // --- 4. Update the derivative states for the NEXT cycle using NEW parameter weights ---
    if constexpr( CP::A_WR ) {
      cur_wr0 = expf(clamp(wr0_state.val + wr1_state.val, CP::UVLO, CP::UVHI));
      cur_wr1 = expf(clamp(wr0_state.val - wr1_state.val, CP::UVLO, CP::UVHI));
    } else {
      cur_wr0 = CP::W0;
      cur_wr1 = CP::W1;
    }
    
    // Calculate the actual retention weights (1.0 - decay_rate), for an
    // observation of weight g
    float w0_retention = 1.0f - cur_wr0*g;
    float w1_retention = 1.0f - cur_wr1*g;

    float w0b = (bit == 0) ? w0_retention : w1_retention;
    float w1b = (bit == 0) ? w1_retention : w0_retention;

    if constexpr( CP::A_WR ) {
    // Applying the chain rule for d/d(alpha). Since (1 - alpha) handles retention, 
    // the injected variable acts negatively in the derivative: d/d_alpha (1 - alpha)*n = -n
    if (bit == 0) {
      rt.n0_ww0 = rt.n0_ww0 * w0_retention - 2.0f * rt.n0_w0 * g;
      rt.n0_ww1 = rt.n0_ww1 * w0_retention;
      rt.n1_ww0 = rt.n1_ww0 * w1_retention;
      rt.n1_ww1 = rt.n1_ww1 * w1_retention - 2.0f * rt.n1_w1 * g;

      rt.n0_w0 = rt.n0_w0 * w0_retention - n0 * g;
      rt.n0_w1 = rt.n0_w1 * w0_retention;
      rt.n1_w0 = rt.n1_w0 * w1_retention;
      rt.n1_w1 = rt.n1_w1 * w1_retention - n1 * g;
    } else {
      rt.n0_ww0 = rt.n0_ww0 * w1_retention;
      rt.n0_ww1 = rt.n0_ww1 * w1_retention - 2.0f * rt.n0_w1 * g;
      rt.n1_ww0 = rt.n1_ww0 * w0_retention - 2.0f * rt.n1_w0 * g;
      rt.n1_ww1 = rt.n1_ww1 * w0_retention;

      rt.n0_w0 = rt.n0_w0 * w1_retention;
      rt.n0_w1 = rt.n0_w1 * w1_retention - n0 * g;
      rt.n1_w0 = rt.n1_w0 * w0_retention - n1 * g;
      rt.n1_w1 = rt.n1_w1 * w0_retention;
    }
    }  // ADAPT_WR

    // --- 5. Update n0 and n1 model values ---
    n0 = n0 * w0b + (1.0f - float(bit)) * g;
    n1 = n1 * w1b + float(bit) * g;


    //const float leakage1 = 0.998f;
    //const float leakage2 = 0.990f;
    if constexpr( CP::A_WR ) {
    rt.n0_w0 *= CP::leakage1; rt.n0_w1 *= CP::leakage1; rt.n1_w0 *= CP::leakage1; rt.n1_w1 *= CP::leakage1;
    rt.n0_ww0 *= CP::leakage2; rt.n0_ww1 *= CP::leakage2; rt.n1_ww0 *= CP::leakage2; rt.n1_ww1 *= CP::leakage2;
    }
  }

}; // struct

#include "sh_SSE2.inc"
#include "sh_mix2.inc"

#ifdef USE_NEW
#if USE_NEW
#define constexpr
#endif
#endif

// Cell types and table sizes of the two stages, then the generated structs
// that hold their Table() storage: S0_T::S0_tbl / M0_T::M0_tbl are fixed
// arrays in the shipping build and pointers allocated by S0_Init()/M0_Init()
// in the tuning build.
typedef Counter<CP_S0> SSE_Cell;
static constexpr qword sse_table_cells( qword volume ) {
  return CP_S0::ON ? sse_rows( volume, CP_S0::HBITS, sse_nb_clamp(CP_S0::NB) ) * qword(sse_nb_clamp(CP_S0::NB)) : 1;
}
typedef Mix2<CP_M0>::Cell Mix2_Cell;
static constexpr uint mix_table_ctx( qword volume ) {
  return CP_M0::ON ? mix_rows( volume ) : 1;
}

#undef constexpr

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
      float p1 = o1[last_c][cxt].PredictF();
      qword cx = qword(S0_MakeCx(c2, last_c, cxt))*S0_Cx3_Volume + S0_MakeCx3(c3);
      float p2 = sse.Predict( cx, p1 );
      float pf = mix.Mix( M0_MakeCx(c2, last_c, cxt), p1, p2 );
      p = uint( clamp( pf*float(SCALE) ) );
      
      bit = rc.rc_BProcess( p, bit );

      // A2: e_f = p_f - [bit==0] = dL/dz_f; chain to the SSE output and the
      // order-1 logit (the SSE input also moves the interpolation point).
      float e_f = pf - float(1 - bit);
      float wv  = mix.weight();
      o1[last_c][cxt].C_Update( bit, p1, 1.0f, e_f * (wv + (1.0f - wv) * sse.dz2_dz1()) );
      sse.Update( bit, e_f * (1.0f - wv) );
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
