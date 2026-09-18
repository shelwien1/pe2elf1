
// -------------------------------------------------------------
// config.hpp -- parameter bundle for Counter<CP>.
//
// Included once per counter component, with that component's generated IDX
// constants already in scope (MOD/sh_model-<prefix>_h.inc):
//
//   #define CP_NAME     CP_C0   // name of the bundle struct to define
//   #define CP_PFX      C0_     // prefix of the IDX constants (C0_wr, ...)
//   #define CP_ADAPT_WR 1       // per-cell adaptive wr (u/v) + RTRL traces
//   #define CP_ADAPT_MW 1       // per-cell adaptive mw
//   #define CP_ADAPT_K  1       // per-cell adaptive K
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
  enum { A_WR = CP_ADAPT_WR, A_MW = CP_ADAPT_MW, A_K = CP_ADAPT_K };   // adaptation flags (names avoid the ADAPT_* macros)

  def_Config(Config_MW)
  def_Config(Config_K)
  def_Config(Config_U)
  def_Config(Config_V)

  static const float stP_min, leakage1, leakage2, mwP0;
  // Float equivalents utilizing delta decoding for P1
  static const float F0_P0, F0_P1_raw, F0_P1;
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

#ifdef CP_SSE
  // SSE stage (sh_SSE2.inc): table geometry and output knobs.  In the
  // shipping build these are constant expressions -- NB is the template
  // argument of SSE_Ctr -- in the tuning build they are runtime values that
  // opt.pl patches, and SSE_Dyn dispatches on NB at Init().
  static const int   NB, HBITS, ILOG, BLOG, UPD, QLIN, HW, HMODE, DEG;
  static const float LIM, T0, W, UPMIN;
#endif
};

#define set  const float CP_NAME::Config_MW::
set momentum_D = 1.0f-float(CPX(M1_m))/(SCALE<<8);
set momentum_R = 1.0f-float(CPX(M2_m))/(SCALE<<8);
set NW         = float(CPX(NWm))/(SCALE<<8);
set inc        = float(CPX(MWinc)) / (SCALE<<8);
set stepMax    = float(CPX(mwStep)) / (SCALE<<8);
set minVal     = float(CPX(mwMin)) / (SCALE<<3);
set maxVal     = float(CPX(mwMax)) / (SCALE<<3);
set grad1_clip = float(CPX(G1_m));
set grad2_clip = float(CPX(G2_m))/(1<<5);
set D_clip = float(CPX(G3_m));
set R_clip = float(CPX(G4_m));
set R0 = 0.0f;
#undef set

#define set  const float CP_NAME::Config_K::
set momentum_D = 1.0f-float(CPX(M1_k))/(SCALE<<8);
set momentum_R = 1.0f-float(CPX(M2_k))/(SCALE<<8);
set NW         = float(CPX(NWk)) / (SCALE<<8);
set inc        = float(CPX(RKinc)) / (SCALE<<8);
set stepMax    = float(CPX(kStep)) / (SCALE<<8);
set minVal     = float(CPX(kMin)) / (SCALE<<8);
set maxVal     = float(CPX(kMax)) / (SCALE>>2);
set grad1_clip = float(CPX(G1_k));
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
set momentum_D = 1.0f-float(CPX(M1_u))/(SCALE<<8);
set momentum_R = 1.0f-float(CPX(M2_u))/(SCALE<<8);
set NW         = float(CPX(NWu))/(SCALE<<8);
set inc        = float(CPX(RUinc)) / (SCALE<<8);
set stepMax    = float(CPX(uStep)) / (SCALE<<8);
set minVal     = float(CPX(uMin)) / (SCALE<<3);
set maxVal     = float(CPX(uMax)) / (SCALE<<8);
set grad1_clip = float(CPX(G1_u));
set grad2_clip = float(CPX(G2_u))/(1<<5);
set D_clip = float(CPX(G3_u));
set R_clip = float(CPX(G4_u));
set R0 = float(CPX(R0_u))/256;
#undef set

#define set  const float CP_NAME::Config_V::
set momentum_D = 1.0f-float(CPX(M1_v))/(SCALE<<8);
set momentum_R = 1.0f-float(CPX(M2_v))/(SCALE<<8);
set NW         = float(CPX(NWv))/(SCALE<<8);
set inc        = float(CPX(RVinc)) / (SCALE<<8);
set stepMax    = float(CPX(vStep)) / (SCALE<<8);
set minVal     = float(CPX(uMin)) / (SCALE<<3);
set maxVal     = float(CPX(uMax)) / (SCALE<<8);
set grad1_clip = float(CPX(G1_v));
set grad2_clip = float(CPX(G2_v))/(1<<5);
set D_clip = float(CPX(G3_v));
set R_clip = float(CPX(G4_v));
set R0 = float(CPX(R0_v))/256;
#undef set

#define set  const float CP_NAME::
set stP_min  = float(CPX(stP_min)) / float(SCALE);
set leakage1 = float(CPX(leak1)) / float(SCALE);
set leakage2 = float(CPX(leak2)) / float(SCALE);
set mwP0     = float(CPX(mwP0)) / float(SCALE);

set F0_P0     = float(2*CPX(P0)) / float(SCALE);
set F0_P1_raw = float(2 * (CPX(P0) + ((CPX(P1) & 1) ? -int(CPX(P1) >> 1) : int(CPX(P1) >> 1)))) / float(SCALE);
set F0_P1     = (CP_NAME::F0_P1_raw < 0.0f) ? 0.0f : CP_NAME::F0_P1_raw;

set W0_raw = float(CPX(wr)) / float(SCALE);
set W0     = clamp(CP_NAME::W0_raw, 0.0f, 1.0f);
set W1_raw = float(CPX(wr) + ((CPX(wr1) & 1) ? -int(CPX(wr1) >> 1) : int(CPX(wr1) >> 1))) / float(SCALE);
set W1     = clamp(CP_NAME::W1_raw, 0.0f, 1.0f);

set M = float(CPX(mw))/SCALE;
set K = float(CPX(K))/SCALE;

// LOGWR/UVROT seeds and log-space box (dynamic init, order matters)
set LW0   = logf(CP_NAME::W0);
set LW1   = logf(CP_NAME::W1);
set UV_U0 = 0.5f*(CP_NAME::LW0+CP_NAME::LW1);   // mean log-decay seed
set UV_V0 = 0.5f*(CP_NAME::LW0-CP_NAME::LW1);   // hit/miss asymmetry seed
set UVLO  = logf(CP_NAME::Config_U::minVal);
set UVHI  = logf(CP_NAME::Config_U::maxVal);
set UV_VH = 0.5f*(CP_NAME::UVHI-CP_NAME::UVLO)*(float(CPX(UVH))/1024);   // |v| bound

set CXW   = float(CPX(CXW))/1024;
set XHW   = float(CPX(XHW) - 1024)/1024;   // signed, seed < 0 per r7 evidence
set UVDET = float(CPX(UVDET))/1024;
set XHC   = float(CPX(XHC))/(1<<5);
set iStepU = 1.0f/(float(CPX(uStep))/(SCALE<<8));
set iStepV = 1.0f/(float(CPX(vStep))/(SCALE<<8));

set MWspan = CP_NAME::Config_MW::maxVal - CP_NAME::Config_MW::minVal;
set MWs0   = (CP_NAME::M - CP_NAME::Config_MW::minVal) / CP_NAME::MWspan;
set MWX0   = logf(CP_NAME::MWs0/(1.0f-CP_NAME::MWs0));
set MWXLO  = float(CPX(mwXlo) - 8192)/1024;   // signed
set MWXHI  = float(CPX(mwXhi) - 8192)/1024;   // signed
set KY0    = logf(CP_NAME::K);
set KYLO   = logf(CP_NAME::Config_K::minVal);
set KYHI   = logf(CP_NAME::Config_K::maxVal);
#undef set

#ifdef CP_SSE
#define set  const int CP_NAME::
set NB    = CPX(NB);
set HBITS = CPX(HBITS);
set ILOG  = CPX(ILOG);
set BLOG  = CPX(BLOG);
set UPD   = CPX(UPD);
set QLIN  = CPX(QLIN);
set HW    = CPX(HW);
set HMODE = CPX(HMODE);
set DEG   = CPX(DEG);
#undef set
#define set  const float CP_NAME::
set LIM   = float(CPX(LIM)) / 256;     // |stretch| clip
set T0    = float(CPX(T0)) / 256;      // initial cell mass
set W     = float(CPX(W)) / SCALE;     // weight of the SSE output
set UPMIN = float(CPX(UPMIN)) / 256;   // floor of the proportional update weight
#undef set
#endif

#undef CPX
#undef CP_CAT
#undef CP_CAT2
#undef CP_NAME
#undef CP_PFX
#undef CP_ADAPT_WR
#undef CP_ADAPT_MW
#undef CP_ADAPT_K
#undef CP_SSE
