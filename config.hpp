
// -------------------------------------------------------------
// config.hpp -- parameter bundle for Counter<CP>.
//
// Included once per counter component, with that component's generated IDX
// constants already in scope (MOD/sh_model-<prefix>_h.inc):
//
//   #define CP_NAME     CP_C0   // name of the bundle struct to define
//   #define CP_PFX      C0_     // prefix of the IDX constants (C0_wr, ...)
//   #define CP_CACHE_WR 1       // cache the post-step wr pair in the cell (+8 B)
//   #define CP_SSE      1       // also derive the SSE-stage knobs (S0 only)
//   #include "config.hpp"
//
// The bundle holds the four ParamUpdater configs (Config_U/V/MW/K) and every
// derived constant Counter reads.  Every formula is the one the pre-template
// coder0 had as a file-scope static; only the spelling moved into a struct so
// that a second component (the SSE cells, prefix S0_) can carry its own set
// and be tuned independently of the order-1 model.
//
// Dynamic-init order matters in the tuning build (the C0_* values are then
// runtime reads of the patchable mapping objects): definitions below are in
// dependency order, exactly as the statics were.
// -------------------------------------------------------------

#define CP_CAT2(a,b) a##b
#define CP_CAT(a,b)  CP_CAT2(a,b)
#define CPX(n)       CP_CAT(CP_PFX, n)

struct CP_NAME {
  enum { CACHE_WR = CP_CACHE_WR };   // keep the post-step wr pair in the cell

  def_Config(Config_MW)
  def_Config(Config_K)
  def_Config(Config_U)
  def_Config(Config_V)

  static const float stP_min, leakage1, leakage2, mwP0;
#ifndef CP_SSE
  // Counter::Init() seeds, from the P0/P1 knobs.  Only the order-1 cells use
  // them: the SSE cells are seeded per bucket through InitN(), so their
  // bundle carries no P0/P1 and Counter<CP_S0>::Init() is never instantiated.
  static const float F0_P0, F0_P1_raw, F0_P1;
#endif
  // W0 and W1 natively track the DECAY rate (alpha)
  static const float W0_raw, W0, W1_raw, W1, M, K;
  // LOGWR/UVROT seeds and log-space box
  static const float LW0, LW1, UV_U0, UV_V0, UVLO, UVHI, UV_VH;
  // 2.1 UV2X2/XHESS/RAYCL: cross-EMA weight, signed h00-h11 shaper, det guard,
  // dedicated clip for the cross-Hessian channel, 1/stepMax for the ray clip
  static const float CXW, XHW, UVDET, XHC, iStepU, iStepV;
  // P24 MWLGT: mw = mwMin + span*sigma(x), K = exp(y); seeds map the linear
  // seeds, x gets its own (wide) box, y reuses the exact ln K box.
  static const float MWspan, MWs0, MWX0, MWXLO, MWXHI, KY0, KYLO, KYHI;
  // A1 young-cell step schedule: stepMax*(1 + AGAx/(1 + age*AGiB)) per axis
  // (u/v share AGAu on the ray clip); AGiB = 1/B with B = AGB/16 updates.
  static const float AGAu, AGAm, AGAk, AGiB;
  // A2 end-to-end objective: the gradient uses (1-E2E)*own error + E2E*final
  // error chained to this cell.
  static const float E2E;

#ifdef CP_SSE
  // SSE stage (sh_SSE.inc): the knobs, clamped here so that the row only
  // ever sees sane values (IDX-FORMAT.md sec.5), and what the row derives
  // from them.  In the shipping build these are constant expressions -- NB
  // is the row's template argument -- in the tuning build they are runtime
  // values that opt.pl patches, and SSE_Tbl dispatches on NB at Init().
  static const int   NB;               // buckets per row, the raw knob: sse_nb_clamp()ed where it is used
  static const int   HBITS;            // log2 of the row cap: the coder hashes the context onto min(volume, 2^HBITS, 2^SSE_MAXCELLS_LOG/NB) rows
  static const int   UPD;              // 1: both bracketing cells get a full event; 2: proportional
  static const float LIM;              // |stretch| clip of the input, in [0.25, 16]
  static const float ZMAX;             // |logit| bound of the cells' outputs, = ln 65535
  static const float QSCALE;           // buckets per unit stretch: (NB-1) / (2 LIM)
  static const float UPMIN;            // floor of the proportional update weights, in [0, 1]
  static const float T0;               // mass n0+n1 of a fresh cell, in [1/256, 4096]
  static const SSE_Seeds SEEDS;        // InitN counts of bucket j, for the clamped NB
#endif
};

#define set  const float CP_NAME::Config_MW::
const int CP_NAME::Config_MW::NAG = 0;   // plain Newton step (the tuned counters)
set momentum_D = 1.0f-float(CPX(M1_m))/(SCALE<<8);
set momentum_R = 1.0f-float(CPX(M2_m))/(SCALE<<8);
set NW         = float(CPX(NWm))/(SCALE<<8);
set inc        = float(CPX(MWinc)) / (SCALE<<8);
set stepMax    = float(CPX(mwStep)) / (SCALE<<8);
set minVal     = float(CPX(mwMin)) / (SCALE<<3);
set maxVal     = float(CPX(mwMax)) / (SCALE<<3);
set grad2_clip = float(CPX(G2_m))/(1<<5);
set D_clip = float(CPX(G3_m));
set R_clip = float(CPX(G4_m));
set R0 = 0.0f;
#undef set

#define set  const float CP_NAME::Config_K::
const int CP_NAME::Config_K::NAG = 0;   // plain Newton step (the tuned counters)
set momentum_D = 1.0f-float(CPX(M1_k))/(SCALE<<8);
set momentum_R = 1.0f-float(CPX(M2_k))/(SCALE<<8);
set NW         = float(CPX(NWk)) / (SCALE<<8);
set inc        = float(CPX(RKinc)) / (SCALE<<8);
set stepMax    = float(CPX(kStep)) / (SCALE<<8);
set minVal     = float(CPX(kMin)) / (SCALE<<8);
set maxVal     = float(CPX(kMax)) / (SCALE>>2);
set grad2_clip = float(CPX(G2_k))/(1<<5);
set D_clip = float(CPX(G3_k));
set R_clip = float(CPX(G4_k));
set R0 = 0.0f;
#undef set

// -------------------------------------------------------------
// LOGWR/UVROT: wr is optimized in log space, in the rotated basis
//   u = (ln wr0 + ln wr1)/2   (memory length)
//   v = (ln wr0 - ln wr1)/2   (hit/miss asymmetry)
// wr0_state tracks u (Config_U), wr1_state tracks v (Config_V, a
// slower dedicated optimizer). Effective wr = exp(clamp(u+-v)).
// RTRL traces stay in linear wr space; the chain to log coordinates
// (dp/du = dp/dwr*wr, d2p/du2 = d2p/dwr2*wr^2 + dp/dwr*wr) is applied
// at gradient-assembly time each step.
// -------------------------------------------------------------

#define set  const float CP_NAME::Config_U::
const int CP_NAME::Config_U::NAG = 0;   // plain Newton step (the tuned counters)
set momentum_D = 1.0f-float(CPX(M1_u))/(SCALE<<8);
set momentum_R = 1.0f-float(CPX(M2_u))/(SCALE<<8);
set NW         = float(CPX(NWu))/(SCALE<<8);
set inc        = float(CPX(RUinc)) / (SCALE<<8);
set stepMax    = float(CPX(uStep)) / (SCALE<<8);
set minVal     = float(CPX(uMin)) / (SCALE<<3);
set maxVal     = float(CPX(uMax)) / (SCALE<<8);
set grad2_clip = float(CPX(G2_u))/(1<<5);
set D_clip = float(CPX(G3_u));
set R_clip = float(CPX(G4_u));
set R0 = float(CPX(R0_u))/256;
#undef set

#define set  const float CP_NAME::Config_V::
const int CP_NAME::Config_V::NAG = 0;   // plain Newton step (the tuned counters)
set momentum_D = 1.0f-float(CPX(M1_v))/(SCALE<<8);
set momentum_R = 1.0f-float(CPX(M2_v))/(SCALE<<8);
set NW         = float(CPX(NWv))/(SCALE<<8);
set inc        = float(CPX(RVinc)) / (SCALE<<8);
set stepMax    = float(CPX(vStep)) / (SCALE<<8);
set minVal     = float(CPX(uMin)) / (SCALE<<3);
set maxVal     = float(CPX(uMax)) / (SCALE<<8);
set grad2_clip = float(CPX(G2_v))/(1<<5);
set D_clip = float(CPX(G3_v));
set R_clip = float(CPX(G4_v));
set R0 = float(CPX(R0_v))/256;
#undef set

#define set  const float CP_NAME::
set stP_min  = float(CPX(stP_min)) / float(SCALE);
// RTRL trace leaks: clamped to [0, 1] at the point of use (IDX-FORMAT.md
// sec.5).  A leak above 1 amplifies the traces until they overflow to inf
// (seen with C0_leak2 = 1.010 after a tuner pass: the ww traces of a
// long-lived cell reached inf, and the shipping and tuning builds then
// differed in how clip() treats inf/NaN under -ffinite-math-only).
set leakage1 = clamp( float(CPX(leak1)) / float(SCALE), 0.0f, 1.0f );
set leakage2 = clamp( float(CPX(leak2)) / float(SCALE), 0.0f, 1.0f );
set mwP0     = float(CPX(mwP0)) / float(SCALE);

#ifndef CP_SSE
set F0_P0     = float(2*CPX(P0)) / float(SCALE);
set F0_P1_raw = float(2 * (CPX(P0) + ((CPX(P1) & 1) ? -int(CPX(P1) >> 1) : int(CPX(P1) >> 1)))) / float(SCALE);
set F0_P1     = (CP_NAME::F0_P1_raw < 0.0f) ? 0.0f : CP_NAME::F0_P1_raw;
#endif

set W0_raw = float(CPX(wr)) / float(SCALE);
set W0     = clamp(CP_NAME::W0_raw, 0.0f, 1.0f);
set W1_raw = float(CPX(wr) + ((CPX(wr1) & 1) ? -int(CPX(wr1) >> 1) : int(CPX(wr1) >> 1))) / float(SCALE);
set W1     = clamp(CP_NAME::W1_raw, 0.0f, 1.0f);

set M = float(CPX(mw))/SCALE;
set K = float(CPX(K))/SCALE;

// LOGWR/UVROT seeds and log-space box (dynamic init, order matters).
// Everything that goes through a log is floored first: the optimizer may
// set a rate or a box edge to 0, and log(0) = -inf would seed every cell
// with -inf (IDX-FORMAT.md sec.5: consumers clamp).  No-ops at the tuned
// values.
#define CP_LOGMIN (1.0f/(1<<24))
set LW0   = rt_logf(fmaxf(CP_NAME::W0, CP_LOGMIN));
set LW1   = rt_logf(fmaxf(CP_NAME::W1, CP_LOGMIN));
set UV_U0 = 0.5f*(CP_NAME::LW0+CP_NAME::LW1);   // mean log-decay seed
set UV_V0 = 0.5f*(CP_NAME::LW0-CP_NAME::LW1);   // hit/miss asymmetry seed
set UVLO  = rt_logf(fmaxf(CP_NAME::Config_U::minVal, CP_LOGMIN));
set UVHI  = rt_logf(fmaxf(CP_NAME::Config_U::maxVal, CP_LOGMIN));
set UV_VH = 0.5f*(CP_NAME::UVHI-CP_NAME::UVLO)*(float(CPX(UVH))/1024);   // |v| bound

set CXW   = float(CPX(CXW))/1024;
set XHW   = float(CPX(XHW) - 1024)/1024;   // signed, seed < 0 per r7 evidence
set UVDET = float(CPX(UVDET))/1024;
set XHC   = float(CPX(XHC))/(1<<5);
set iStepU = 1.0f/(float(CPX(uStep))/(SCALE<<8));
set iStepV = 1.0f/(float(CPX(vStep))/(SCALE<<8));

// mw box and logit seed: a seed on or beyond the box edge (mw >= mwMax,
// mw <= mwMin, or an empty box) would give MWX0 = log(<= 0) = NaN and every
// cell a NaN mw; clamp the seed inside as SSE_Ctr::Init does for q0.
set MWspan = fmaxf(CP_NAME::Config_MW::maxVal - CP_NAME::Config_MW::minVal, 1.0f/4096);
set MWs0   = clamp((CP_NAME::M - CP_NAME::Config_MW::minVal) / CP_NAME::MWspan, 1.0f/4096, 1.0f-1.0f/4096);
set MWX0   = rt_logf(CP_NAME::MWs0/(1.0f-CP_NAME::MWs0));
set MWXLO  = float(CPX(mwXlo) - 8192)/1024;   // signed
set MWXHI  = float(CPX(mwXhi) - 8192)/1024;   // signed
set KY0    = rt_logf(fmaxf(CP_NAME::K, CP_LOGMIN));
set KYLO   = rt_logf(fmaxf(CP_NAME::Config_K::minVal, CP_LOGMIN));
set KYHI   = rt_logf(fmaxf(CP_NAME::Config_K::maxVal, CP_LOGMIN));
#undef CP_LOGMIN

set AGAu   = float(CPX(AGAu))/256;
set AGAm   = float(CPX(AGAm))/256;
set AGAk   = float(CPX(AGAk))/256;
set AGiB   = 16.0f/float(CPX(AGB) < 1 ? 1 : CPX(AGB));   // never divides by 0
set E2E    = clamp( float(CPX(E2E))/256, 0.0f, 1.0f );
#undef set

#ifdef CP_SSE
#define set  const int CP_NAME::
set NB    = CPX(NB);
set HBITS = CPX(HBITS);
set UPD   = CPX(UPD) < 1 ? 1 : CPX(UPD) > 2 ? 2 : CPX(UPD);
#undef set
#define set  const float CP_NAME::
set LIM    = clamp( float(CPX(LIM)) / 256, 0.25f, 16.0f );
set ZMAX   = rt_logf( 65535.0f );
set QSCALE = float(sse_nb_clamp(CP_NAME::NB)-1) / (2.0f*CP_NAME::LIM);
set UPMIN  = clamp( float(CPX(UPMIN)) / 256, 0.0f, 1.0f );
set T0     = clamp( float(CPX(T0)) / 256, 1.0f/256, 4096.0f );
#undef set
// the identity init of a fresh row (sse_seeds, coder0.cpp), last: it reads
// K, M, mwP0, LIM, T0 and NB above
const SSE_Seeds CP_NAME::SEEDS = sse_seeds( sse_nb_clamp(CP_NAME::NB), CP_NAME::LIM, CP_NAME::K, CP_NAME::M, CP_NAME::mwP0, CP_NAME::T0 );
#endif

#undef CPX
#undef CP_CAT
#undef CP_CAT2
#undef CP_NAME
#undef CP_PFX
#undef CP_CACHE_WR
#undef CP_SSE
