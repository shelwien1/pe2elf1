
// -------------------------------------------------------------
// sh_counter0.inc -- coder0's adaptive binary counter and the
// ParamUpdater that drives it, split out of coder0.cpp verbatim so
// that other codecs (bmpc.cpp) can use the same machinery.
// Requires sh_common.inc first.
//
// The ADAPT_*/OPT_* toggles keep their coder0 defaults; a consumer
// may -D them or #define them before including this file.
// -------------------------------------------------------------

// -------------------------------------------------------------
// Toggles for enabling/disabling parameter adaptations.
// Set to 0 to disable an update entirely (saves memory & compute).
// -------------------------------------------------------------
#ifndef ADAPT_WR
#define ADAPT_WR 1
#endif
#ifndef ADAPT_MW
#define ADAPT_MW 1
#endif
#ifndef ADAPT_K
#define ADAPT_K  1
#endif

// -------------------------------------------------------------
// Optimizer-proposal toggles (coder0_counter_scope.md / r8 doc).
// Every toggle defaults to 0 == bit-identical to the 050b-opt baseline.
// All changes live in Counter / ParamUpdater + new constants only.
// -------------------------------------------------------------
// 050c results (book1/wcc386/book1_wcc386 vs 050b 345333/310485/657402):
//   final: 344956/310144/656681 = -377/-341/-721; decode verified.
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
// d2p/ww note: with C0_G2_u=C0_G2_v=0 the u/v d2p channel clips to 0, so all
//   n?_ww? traces stay dead code; reviving via G2>0 costs +110..+155 concat.
// Baseline recovery: -DOPT_MWLGT=0 -DOPT_KLOG=0 -DOPT_UV2X2=0 -DOPT_DIAMP=0
//   plus 050b values of the 16 retuned constants (kept in .inc comments).
// -------------------------------------------------------------
#ifndef OPT_UV2X2
#define OPT_UV2X2 1  // 2.1: coupled 2x2 (u,v) solve + XHESS cross + RAYCL (+1 float)
#endif
#ifndef OPT_MKCPL
#define OPT_MKCPL 0  // P10: coupled 2x2 (mw,K) solve, R_mk EMA cross (+1 float)
#endif

#include "MOD/sh_model-C0_h.inc"

#include "config.hpp"

static const float stP_min = float(C0_stP_min) / float(SCALE);
static const float leakage1 = float(C0_leak1) / float(SCALE);
static const float leakage2 = float(C0_leak2) / float(SCALE);
static const float mwP0 = float(C0_mwP0) / float(SCALE);

// Float equivalents utilizing delta decoding for P1
static const float F0_P0 = float(2*C0_P0) / float(SCALE);
static const float F0_P1_raw = float(2 * (C0_P0 + ((C0_P1 & 1) ? -int(C0_P1 >> 1) : int(C0_P1 >> 1)))) / float(SCALE);
static const float F0_P1 = (F0_P1_raw < 0.0f) ? 0.0f : F0_P1_raw;

// W0 and W1 now natively track the DECAY rate (alpha). Scale subtraction is removed.
static const float W0_raw = float(C0_wr) / float(SCALE);
static const float W0 = clamp(W0_raw, 0.0f, 1.0f);

static const float W1_raw = float(C0_wr + ((C0_wr1 & 1) ? -int(C0_wr1 >> 1) : int(C0_wr1 >> 1))) / float(SCALE);
static const float W1 = clamp(W1_raw, 0.0f, 1.0f);

static const float M = float(C0_mw)/SCALE;
static const float K = float(C0_K)/SCALE;

// LOGWR/UVROT seeds and log-space box (dynamic init, order matters)
static const float LW0   = logf(W0);
static const float LW1   = logf(W1);
static const float UV_U0 = 0.5f*(LW0+LW1);   // mean log-decay seed
static const float UV_V0 = 0.5f*(LW0-LW1);   // hit/miss asymmetry seed
static const float UVLO  = logf(Config_U::minVal);
static const float UVHI  = logf(Config_U::maxVal);
static const float UV_VH = 0.5f*(UVHI-UVLO)*(float(C0_UVH)/1024);   // |v| bound

// --- Derived constants for the r8/counter-scope proposals ---
// P4 AWIND: momentum bleed factors (1.0 == off)
static const float AWF   = float(C0_AWF)/1024;
static const float AWFo  = float(C0_AWFo)/1024;
// 2.1 UV2X2/XHESS/RAYCL: cross-EMA weight, signed h00-h11 shaper, det guard,
// dedicated clip for the cross-Hessian channel, 1/stepMax for the ray clip
static const float CXW   = float(C0_CXW)/1024;
static const float XHW   = float(C0_XHW - 1024)/1024;   // signed, seed < 0 per r7 evidence
static const float UVDET = float(C0_UVDET)/1024;
static const float XHC   = float(C0_XHC)/(1<<5);
static const float iStepU = 1.0f/(float(C0_uStep)/(SCALE<<8));
static const float iStepV = 1.0f/(float(C0_vStep)/(SCALE<<8));
// P10 MKCPL constants (statics valid for both coordinate systems; the ray
// clip inverses are against the (x,y) stepMax which equal the class stepMax).
static const float CMK   = float(C0_CMK)/1024;
static const float iStepM = 1.0f/(float(C0_mwStep)/(SCALE<<8));
static const float iStepK = 1.0f/(float(C0_kStep)/(SCALE<<8));
static const float MKDET = float(C0_MKDET)/1024;
// P9 ALEAK: leak1_eff = 1 - clamp(LKA*wr_geo + LKB, LKlo, LKhi)
static const float LKA   = float(C0_LKA)/4096;
static const float LKB   = float(C0_LKB)/(SCALE<<2);
static const float LKlo  = float(C0_LKlo)/(SCALE<<2);
static const float LKhi  = float(C0_LKhi)/(SCALE<<2);
// 3 SMASS: mass conditioning of mw/K; MPW ramps the MPATH correction
static const float SMW_M = float(C0_SMWm - 8192)/(SCALE<<2);   // signed
static const float SMW_K = float(C0_SMWk - 8192)/(SCALE<<2);   // signed
static const float SM0   = float(C0_SM0)/(SCALE>>3);
static const float MPW   = float(C0_MPW)/1024;
// P18 BIASC: warmup ramp n_sum/(n_sum+BWT)
static const float BWT   = float(C0_BWT)/(SCALE>>3);
// P24 MWLGT: mw = mwMin + span*sigma(x), K = exp(y); seeds map the current
// linear seeds, x gets its own (wide) box, y reuses the exact ln K box.
static const float MWspan = Config_MW::maxVal - Config_MW::minVal;
static const float MWs0   = (::M - Config_MW::minVal) / MWspan;
static const float MWX0   = logf(MWs0/(1.0f-MWs0));
static const float MWXLO  = float(C0_mwXlo - 8192)/1024;   // signed
static const float MWXHI  = float(C0_mwXhi - 8192)/1024;   // signed
static const float KY0    = logf(::K);
static const float KYLO   = logf(Config_K::minVal);
static const float KYHI   = logf(Config_K::maxVal);

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

  INLINE void Accum(
    float dp, float d2p, float inv_p, float inv_p2, float inv_pq
  ) {
    float dp_inv = dp * inv_p;
    float dp2_inv = (dp * dp) * inv_p2;
    (void)inv_pq;
    float d2p_inv = d2p * inv_p;

    // --- GRADIENT CLIPPING ---
    d2p_inv= clip( d2p_inv, cfg::grad2_clip );
    // -------------------------


    D = D * cfg::momentum_D - dp_inv;
    R = R * cfg::momentum_R + dp2_inv - d2p_inv;

    D = clip(D,cfg::D_clip);
    //R = clip(R,cfg::R_clip);
    R = clamp(R,0.0f,cfg::R_clip);
  }

  // Loss-space entry point.  Accum() above is the likelihood form: it is
  // handed dp/d2p of a probability and builds D = EMA(d(-ln p)/dval),
  // R = EMA(d2(-ln p)/dval2) out of them.  A plain regression loss (bmpc's
  // LPC weights) already has both derivatives, so it skips the 1/p chain and
  // fills the same two accumulators directly -- same momenta, same clips,
  // same StepRaw()/Apply().  Feeding Accum() with inv_p=1, dp=-g,
  // d2p=g*g-h is arithmetically identical; this spells it instead.
  INLINE void AccumGH( float g, float h ) {
    h = clip( h, cfg::grad2_clip );

    D = D * cfg::momentum_D + g;
    R = R * cfg::momentum_R + h;

    D = clip(D,cfg::D_clip);
    R = clamp(R,0.0f,cfg::R_clip);
  }

  INLINE float Denom() const { return R + cfg::inc; }

  INLINE float StepRaw() const {
    //float safe_R = fabsf(R) + cfg::inc;
    //float safe_R = Max(R,0.0f) + cfg::inc;
    float safe_R = R + cfg::inc;
    return cfg::NW * D / safe_R;
  }

  INLINE void Apply( float step, float lo, float hi ) {
    step = clip(step, cfg::stepMax);

    float nv = val - step;
    val = clamp(nv, lo, hi);
  }

  void Update(
    float dp, float d2p, float inv_p, float inv_p2, float inv_pq, float sc,
    float lo, float hi
  ) {
    Accum(dp, d2p, inv_p, inv_p2, inv_pq);
    (void)sc;
    Apply(StepRaw(), lo, hi);
  }

};

// Inactive parameter state
template<class cfg> struct ParamUpdater<0, cfg> {
  void Init(float /*init_val*/) {}
  inline void Accum(float, float, float, float, float) {}
  inline void AccumGH(float, float) {}
  inline float Denom() const { return 1.0f; }
  inline float StepRaw() const { return 0.0f; }
  inline void Apply(float, float, float) {}
  inline void Update(float /*dp*/, float /*d2p*/, float /*inv_p*/, float /*inv_p2*/, float /*inv_pq*/, float /*sc*/, float /*lo*/, float /*hi*/) {}
};


struct Counter {
  float n0;
  float n1;
  float pK;


  // Context-adaptive parameters logic encapsulating both wr limits
  // UVROT: wr0_state tracks u = (ln wr0 + ln wr1)/2,
  //        wr1_state tracks v = (ln wr0 - ln wr1)/2
  ParamUpdater<ADAPT_WR, Config_U>  wr0_state;
  ParamUpdater<ADAPT_WR, Config_V>  wr1_state;
  ParamUpdater<ADAPT_MW, Config_MW> mw_state;
  ParamUpdater<ADAPT_K,  Config_K>  k_state;

  // Real-Time Recurrent Learning helper states for {n0, n1} variables
#if ADAPT_WR
  float n0_w0, n0_ww0, n0_w1, n0_ww1;
  float n1_w0, n1_ww0, n1_w1, n1_ww1;
#endif

#if OPT_UV2X2 && ADAPT_WR
  float R_uv;   // 2.1: EMA of the rotated (u,v) cross curvature (+1 float)
#endif

  float st( const float p_ ) const {
    float p = p_ * 0.999998f + 0.000001f;
    return logf(p/(1.0f-p));
  }

  float sq( const float x ) const {
    return 1.0f/(1.0f+expf(-x));
  }

  void Init() {
    n0 = F0_P0;
    n1 = F0_P1;
    if( n0+n1==0.0f ) {
      n0 = iSCALE; 
      n1 = iSCALE;
    }


    wr0_state.Init(::UV_U0);
    wr1_state.Init(::UV_V0);
    mw_state.Init(::MWX0);
    k_state.Init(::KY0);

#if ADAPT_WR
    n0_w0 = 0.0f; n0_ww0 = 0.0f; n0_w1 = 0.0f; n0_ww1 = 0.0f;
    n1_w0 = 0.0f; n1_ww0 = 0.0f; n1_w1 = 0.0f; n1_ww1 = 0.0f;
#endif

#if OPT_UV2X2 && ADAPT_WR
    R_uv = 0.0f;
#endif
  } 

  float Predict() {
#if ADAPT_MW
    float cur_mw = Config_MW::minVal + MWspan * sq(mw_state.val);
#else
    float cur_mw = ::M;
#endif

#if ADAPT_K
    float curr_K = expf(k_state.val);
#else
    float curr_K = ::K;
#endif

    float n_sum = n0 + n1 + 1e-8f;
    float p0 = n0 / n_sum;
    p0 = p0 * (1.0f - cur_mw) + mwP0 * cur_mw;
    pK = sq(curr_K * st(p0));
    
    float p_out = pK * float(SCALE);
    p_out = clamp(p_out);
    return p_out;
  }

  void C_Update( const int bit ) {
    float sign = 1.0f - float(bit + bit); 

    float pK_t = pK * sign + float(bit);
    if( pK_t < 1e-12f ) pK_t = 1e-12f;
    float inv_pK_t = 1.0f / pK_t;
    float inv_pK_t2 = inv_pK_t * inv_pK_t;

    const float inv_pq = 0.0f;
    const float bias_sc = 1.0f;

#if ADAPT_WR
    float cur_wr0 = expf(clamp(wr0_state.val + wr1_state.val, UVLO, UVHI));
    float cur_wr1 = expf(clamp(wr0_state.val - wr1_state.val, UVLO, UVHI));
#else
    float cur_wr0 = ::W0;
    float cur_wr1 = ::W1;
#endif

#if ADAPT_MW
    float mws = sq(mw_state.val);
    float cur_mw = Config_MW::minVal + MWspan * mws;
#else
    float cur_mw = ::M;
#endif

#if ADAPT_WR || ADAPT_MW
    #if ADAPT_K
        float curr_K = expf(k_state.val);
    #else
        float curr_K = ::K;
    #endif

    float n_sum = n0 + n1 + 1e-8f;
    float q0 = n0 / n_sum;
    float p_mix = q0 * (1.0f - cur_mw) + mwP0 * cur_mw;

    float P_adj = p_mix * 0.999998f + 0.000001f;
    float P_adj_inv = 1.0f / P_adj;
    float P_adj_inv_comp = 1.0f / (1.0f - P_adj);
    float d_st_dP = 0.999998f * (P_adj_inv + P_adj_inv_comp);
    float d2_st_dP2 = (0.999998f * 0.999998f) * (P_adj_inv_comp * P_adj_inv_comp - P_adj_inv * P_adj_inv);

    float dpK_dpmix = pK * (1.0f - pK) * curr_K * d_st_dP;
    float d2pK_dpmix2 = (1.0f - 2.0f * pK) * dpK_dpmix * curr_K * d_st_dP + pK * (1.0f - pK) * curr_K * d2_st_dP2;
#endif

    // --- 1. Update WR0 and WR1 parameters based on current derivatives ---
#if ADAPT_WR
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
    float dq0_dwr0 = dq0_dn0 * n0_w0 + dq0_dn1 * n1_w0;
    float dq0_dwr1 = dq0_dn0 * n0_w1 + dq0_dn1 * n1_w1;

    float d2q0_dwr02 = d2q0_dn02 * (n0_w0*n0_w0) + d2q0_dn12 * (n1_w0*n1_w0) + 2.0f * cross * n0_w0 * n1_w0 
                     + dq0_dn0 * n0_ww0 + dq0_dn1 * n1_ww0;

    float d2q0_dwr12 = d2q0_dn02 * (n0_w1*n0_w1) + d2q0_dn12 * (n1_w1*n1_w1) + 2.0f * cross * n0_w1 * n1_w1
                     + dq0_dn0 * n0_ww1 + dq0_dn1 * n1_ww1;

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
    wr0_state.Accum(g_u * sign, h_d * sign, inv_pK_t, inv_pK_t2, inv_pq);
    wr1_state.Accum(g_v * sign, h_d * sign, inv_pK_t, inv_pK_t2, inv_pq);

    // XHESS: exact rotated cross diagonal h_uv = h00 - h11 (MIXTR-free part),
    // exposed through the signed shaper XHW with its own clip (G2_u/v are 0
    // in the delivered constants, so the cross channel gets a dedicated one).
    float h_x = clip( (d2pK_dwr02 - d2pK_dwr12) * sign * inv_pK_t, XHC );
    R_uv = R_uv * Config_U::momentum_R + CXW * (g_u * g_v * inv_pK_t2 - XHW * h_x);

    {
      float a_u = wr0_state.Denom();
      float a_v = wr1_state.Denom();
      float cc  = R_uv;
      float det = a_u * a_v - cc * cc;
      if( det > UVDET * a_u * a_v ) {
        float b_u = Config_U::NW * wr0_state.D;
        float b_v = Config_V::NW * wr1_state.D;
        float idet = 1.0f / det;
        float s_u = (b_u * a_v - cc * b_v) * idet;
        float s_v = (a_u * b_v - cc * b_u) * idet;
        // RAYCL: rescale jointly so neither component exceeds its stepMax.
        float r = fmaxf( fmaxf( fabsf(s_u)*iStepU, fabsf(s_v)*iStepV ), 1.0f );
        float ir = 1.0f / r;
        wr0_state.Apply(s_u * ir, UVLO, UVHI);
        wr1_state.Apply(s_v * ir, -UV_VH, UV_VH);
      } else {
        wr0_state.Apply(wr0_state.StepRaw(), UVLO, UVHI);
        wr1_state.Apply(wr1_state.StepRaw(), -UV_VH, UV_VH);
      }
    }

    // 2.2 DIAMP: replace the two independent boxes by the clamp in original
    // (ln wr0, ln wr1) coordinates + back-rotation; kills the corner where
    // u,v are individually legal but u+-v is pinned at the exp-time clamp
    // (measured 26-37% of wr updates) while the stored coordinates drift.
    {
      float t0 = clamp(wr0_state.val + wr1_state.val, UVLO, UVHI);
      float t1 = clamp(wr0_state.val - wr1_state.val, UVLO, UVHI);
      float nu = 0.5f * (t0 + t1);
      float nv = 0.5f * (t0 - t1);
      wr0_state.val = nu;
      wr1_state.val = nv;
    }
#endif

    // --- 2.+3. coupled (mw,K) update, P10 MKCPL ---
#if OPT_MKCPL && ADAPT_MW && ADAPT_K
    {
      float dpmix_dmw = mwP0 - q0;
      float dpK_dmw = dpK_dpmix * dpmix_dmw;
      float d2pK_dmw2 = d2pK_dpmix2 * (dpmix_dmw * dpmix_dmw);
      float f1m = MWspan * mws * (1.0f - mws);
      float f2m = f1m * (1.0f - 2.0f * mws);
      float g_m = dpK_dmw * f1m;
      float h_m = d2pK_dmw2 * (f1m * f1m) + dpK_dmw * f2m;
      const float mw_lo = MWXLO, mw_hi = MWXHI;
      float stP = st(p_mix);
      float dpK_dK = pK * (1.0f - pK) * stP;
      float d2pK_dK2 = dpK_dK * (1.0f - 2.0f * pK) * stP;
      float g_k = dpK_dK * curr_K;
      float h_k = d2pK_dK2 * (curr_K * curr_K) + dpK_dK * curr_K;
      const float k_lo = KYLO, k_hi = KYHI;
      if( fabsf(stP)>=stP_min ) {
        // gate open: both rows identifiable -> accumulate, EMA the cross,
        // 2x2 solve with det guard + joint ray clip (UV2X2 machinery).
        mw_state.Accum(g_m * sign, h_m * sign, inv_pK_t, inv_pK_t2, inv_pq);
        k_state.Accum (g_k * sign, h_k * sign, inv_pK_t, inv_pK_t2, inv_pq);
        R_mk = R_mk * Config_MW::momentum_R + CMK * (g_m * g_k * inv_pK_t2);
        float a_m = mw_state.Denom();
        float a_k = k_state.Denom();
        float cc  = R_mk;
        float det = a_m * a_k - cc * cc;
        if( det > MKDET * a_m * a_k ) {
          float b_m = Config_MW::NW * mw_state.D;
          float b_k = Config_K::NW * k_state.D;
          float idet = 1.0f / det;
          float s_m = (b_m * a_k - cc * b_k) * idet;
          float s_k = (a_m * b_k - cc * b_m) * idet;
          mw_state.Apply(s_m, mw_lo, mw_hi);
          k_state.Apply (s_k, k_lo, k_hi);
        } else {
          mw_state.Apply(mw_state.StepRaw(), mw_lo, mw_hi);
          k_state.Apply (k_state.StepRaw(),  k_lo, k_hi);
        }
      } else {
        // gate closed: K row unidentifiable -> independent mw update (doc),
        // cross EMA decays without injection.
        mw_state.Update(g_m * sign, h_m * sign, inv_pK_t, inv_pK_t2, inv_pq, bias_sc, mw_lo, mw_hi);
        R_mk = R_mk * Config_MW::momentum_R;
      }
    }
#else
    // --- 2. Update MW parameter ---
#if ADAPT_MW
    float dpmix_dmw = mwP0 - q0;
    float dpK_dmw = dpK_dpmix * dpmix_dmw;
    float d2pK_dmw2 = d2pK_dpmix2 * (dpmix_dmw * dpmix_dmw);

    // P24: chain to the logit coordinate, mw = lo + span*sigma(x):
    // f' = span*s(1-s), f'' = f'*(1-2s) (the commonly-dropped term kept).
    {
      float f1m = MWspan * mws * (1.0f - mws);
      float f2m = f1m * (1.0f - 2.0f * mws);
      float g_xm = dpK_dmw * f1m;
      float h_xm = d2pK_dmw2 * (f1m * f1m) + dpK_dmw * f2m;
      mw_state.Update(g_xm * sign, h_xm * sign, inv_pK_t, inv_pK_t2, inv_pq, bias_sc, MWXLO, MWXHI);
    }
#endif

    // --- 3. Update K parameter ---
#if ADAPT_K
    float stP = st(p_mix);
    float dpK_dK = pK * (1.0f - pK) * stP;
    float d2pK_dK2 = dpK_dK * (1.0f - 2.0f * pK) * stP;
    
    if( fabsf(stP)>=stP_min ) {
      // P24: chain to y = ln K: f' = f'' = K.
      float g_yk = dpK_dK * curr_K;
      float h_yk = d2pK_dK2 * (curr_K * curr_K) + dpK_dK * curr_K;
      k_state.Update(g_yk * sign, h_yk * sign, inv_pK_t, inv_pK_t2, inv_pq, bias_sc, KYLO, KYHI);
    }
#endif
#endif  // OPT_MKCPL sections 2+3 variant select


    // --- 4. Update the derivative states for the NEXT cycle using NEW parameter weights ---
#if ADAPT_WR
    cur_wr0 = expf(clamp(wr0_state.val + wr1_state.val, UVLO, UVHI));
    cur_wr1 = expf(clamp(wr0_state.val - wr1_state.val, UVLO, UVHI));
#else
    cur_wr0 = ::W0;
    cur_wr1 = ::W1;
#endif
    
    // Calculate the actual retention weights (1.0 - decay_rate)
    float w0_retention = 1.0f - cur_wr0;
    float w1_retention = 1.0f - cur_wr1;

    float w0b = (bit == 0) ? w0_retention : w1_retention;
    float w1b = (bit == 0) ? w1_retention : w0_retention;

#if ADAPT_WR
    // Applying the chain rule for d/d(alpha). Since (1 - alpha) handles retention, 
    // the injected variable acts negatively in the derivative: d/d_alpha (1 - alpha)*n = -n
    if (bit == 0) {
      n0_ww0 = n0_ww0 * w0_retention - 2.0f * n0_w0;
      n0_ww1 = n0_ww1 * w0_retention;
      n1_ww0 = n1_ww0 * w1_retention;
      n1_ww1 = n1_ww1 * w1_retention - 2.0f * n1_w1;

      n0_w0 = n0_w0 * w0_retention - n0;
      n0_w1 = n0_w1 * w0_retention;
      n1_w0 = n1_w0 * w1_retention;
      n1_w1 = n1_w1 * w1_retention - n1;
    } else {
      n0_ww0 = n0_ww0 * w1_retention;
      n0_ww1 = n0_ww1 * w1_retention - 2.0f * n0_w1;
      n1_ww0 = n1_ww0 * w0_retention - 2.0f * n1_w0;
      n1_ww1 = n1_ww1 * w0_retention;

      n0_w0 = n0_w0 * w1_retention;
      n0_w1 = n0_w1 * w1_retention - n0;
      n1_w0 = n1_w0 * w0_retention - n1;
      n1_w1 = n1_w1 * w0_retention;
    }
#endif

    // --- 5. Update n0 and n1 model values ---
    n0 = n0 * w0b + (1.0f - float(bit));
    n1 = n1 * w1b + float(bit);


    //const float leakage1 = 0.998f;
    //const float leakage2 = 0.990f;
#if ADAPT_WR
    n0_w0 *= leakage1; n0_w1 *= leakage1; n1_w0 *= leakage1; n1_w1 *= leakage1;
    n0_ww0 *= leakage2; n0_ww1 *= leakage2; n1_ww0 *= leakage2; n1_ww1 *= leakage2;
#endif
  }

}; // struct
