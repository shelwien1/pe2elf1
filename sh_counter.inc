
// Real-Time Recurrent Learning traces of {n0, n1} w.r.t. wr0/wr1, plus the
// UV2X2 cross-curvature EMA.  Carried by every cell: wr, mw and K adapt in
// both components.  w[i][j] = d n_i / d wr_j and ww[i][j] the second
// derivative: eight floats laid out as one AVX register, since their
// recursion (C_Update, step 4) is the same multiply-subtract on every lane.
struct RTRLState {
  float w[2][2];    // [n0|n1][wr0|wr1]
  float ww[2][2];
  float R_uv;       // 2.1: EMA of the rotated (u,v) cross curvature (+1 float)
};


// Counter<CP>: one adaptive binary probability cell.  CP is a parameter
// bundle made by config.hpp (CP_C0 for the order-1 model, CP_S0 for the SSE
// cells): it carries the four ParamUpdater configs and every derived
// constant the counter reads.  wr (as u/v), mw and K all adapt, and the A2
// end-to-end term is always in the gradient.
template<class CP> struct Counter {
  typedef typename CP::Config_U  Config_U;
  typedef typename CP::Config_V  Config_V;
  typedef typename CP::Config_MW Config_MW;
  typedef typename CP::Config_K  Config_K;

  // Real-Time Recurrent Learning helper states for the counts; first, so
  // that the eight traces sit at the cell's 32-byte boundary
  RTRLState rt;

  float n[2];   // the counts of bit 0 and bit 1
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
  ParamUpdater<Config_U>  wr0_state;
  ParamUpdater<Config_V>  wr1_state;
  ParamUpdater<Config_MW> mw_state;
  ParamUpdater<Config_K>  k_state;

  // What PredictF() computes and C_Update() needs back.  The caller keeps it
  // between the two calls (per-query state), so nothing is stored in the
  // cell and nothing is recomputed: q0, the prior mix, its stretch, the
  // sigmoid of the mw coordinate and exp(K) each cost a division and/or a
  // transcendental, and C_Update used to redo all of them.
  struct Pred {
    float pK;      // P(bit==0), the cell's prediction
    float z;       // its logit K*st(p_mix): pK = sq(z); the stages pass logits on
    float q0;      // n0/(n0+n1)
    float inv_n;   // 1/(n0+n1)
    float p_mix;   // q0 mixed with the prior mwP0 at weight mw
    float stP;     // st(p_mix)
    float mws;     // sigma(mw_state.val); unused when mw is fixed
    float K;       // exp(k_state.val), or the fixed K
  };

  static float st( const float p_ ) {
    float p = p_ * 0.999998f + 0.000001f;
    return ::st_schraudolph(p);   // the bit-difference stretch of schrau.inc, no division
  }

  static float sq( const float x ) {
    return 1.0f/(1.0f+expf(-x));
  }

  // Init with explicit counts; SSE cells start at their bucket's probability.
  void InitN( float a, float b ) {
    n[0] = a;
    n[1] = b;
    if( n[0]+n[1]==0.0f ) {
      n[0] = iSCALE; 
      n[1] = iSCALE;
    }
    age = 0;


    wr0_state.Init(CP::UV_U0);
    wr1_state.Init(CP::UV_V0);
    mw_state.Init(CP::MWX0);
    k_state.Init(CP::KY0);

    for( int i=0; i<2; i++ ) for( int j=0; j<2; j++ ) { rt.w[i][j] = 0.0f; rt.ww[i][j] = 0.0f; }
    rt.R_uv = 0.0f;
  } 

  void Init() {
    InitN( CP::F0_P0, CP::F0_P1 );
  }

  // Current logistic scale
  float cur_K() const { return expf(k_state.val); }

  // The prediction (P(bit==0) in .pK) and its intermediates; the caller
  // keeps the struct for C_Update()
  Pred PredictF() const {
    Pred r;
    r.mws = sq(mw_state.val);
    const float cur_mw = Config_MW::minVal + CP::MWspan * r.mws;
    r.K = this->cur_K();

    float n_sum = n[0] + n[1] + 1e-8f;
    r.inv_n = 1.0f / n_sum;
    r.q0    = n[0] * r.inv_n;
    r.p_mix = r.q0 * (1.0f - cur_mw) + CP::mwP0 * cur_mw;
    r.stP   = st(r.p_mix);
    r.z     = r.K * r.stP;
    r.pK    = sq(r.z);
    return r;
  }

  // pr = this cell's PredictF() for the bit being coded.
  // g = weight of this observation, 1 = a full event (bit-identical to the
  // unweighted update).  A fractional event scales the loss by g, and in the
  // count recursion every wr becomes wr*g and every injected count g, so
  // the RTRL traces stay the exact derivatives of the weighted recursion.
  // ef = A2 end-to-end error: dL_final/dz where z = st(pK) is this cell's
  // logit output, i.e. (p_final - [bit==0]) * dz_final/dz.  The cell's own
  // error is e = pK - [bit==0]; the gradient uses (1-E2E)*e + E2E*ef, the
  // curvature stays the cell's own.  E2E = 0 ignores ef (bit-identical).
  void C_Update( const int bit, const Pred& pr, const float g = 1.0f, const float ef = 0.0f ) {
    const float pK = pr.pK;
    float sign = 1.0f - float(bit + bit); 

    // A1: young-cell step schedule, per axis: stepMax*(1 + AGA*agd),
    // agd = 1/(1 + age/B), B = AGB for mw and K and AGBuv for the u/v ray
    // clip.  AGA = 0 reproduces the plain limit bit for bit.
    float agd   = 1.0f / (1.0f + float(age) * CP::AGiB);
    float agduv = 1.0f / (1.0f + float(age) * CP::AGiBuv);
    if( age != ~0u ) age++;

    // Logit-domain chain.  z = K*st(p_mix) and pK = sq(z), so the loss
    // gradient w.r.t. z is pK - [bit==0] = -e_o with e_o = sign*p_o, p_o the
    // probability of the symbol that did not occur.  For a parameter t with
    // z' = dz/dt and z'' = d2z/dt2, the two terms ParamUpdater takes are
    //   g  = -dL/dt         = e_o*z'                    (was dpK/dt / p_t)
    //   hc = d2pK/dt2 / p_t = e_o*((1-2pK)*z'^2 + z'')
    // -- the p_t of the probability-domain form cancels, so no reciprocal
    // is needed and a surprise (p_t -> 0) inflates nothing: p_o <= 1.
    // A2 blends the cell's error with the final coder's in the gradient,
    // ee = (1-E2E)*e_o - E2E*ef; the curvature stays the cell's own.
    // (Where pK has saturated to exactly 1.0f, |z| > 16.6, the old chain's
    // pK*(1-pK) was 0 and the cell saw no gradient; here e_o*z' is the true
    // one.  Making that case a no-event changed nothing on the corpus.)
    const float p_o = bit ? pK : 1.0f - pK;
    const float e_o = sign * p_o;
    const float ee   = (1.0f - CP::E2E)   * e_o - CP::E2E   * ef;   // mw and K
    const float eeuv = (1.0f - CP::E2Euv) * e_o - CP::E2Euv * ef;   // the u/v axes
    const float c2 = 1.0f - 2.0f * pK;

    // the pre-step wr pair, wr0 = exp(u+v) and wr1 = exp(u-v) (a local: it is
    // what the previous update's post-step pair was, recomputed rather than
    // kept in the cell)
    float cur_wr[2];
    cur_wr[0] = expf(clamp(wr0_state.val + wr1_state.val, CP::UVLO, CP::UVHI));
    cur_wr[1] = expf(clamp(wr0_state.val - wr1_state.val, CP::UVLO, CP::UVHI));

    // from PredictF(): sigma(mw), mw, K, q0 and the prior mix
    const float mws = pr.mws;
    const float cur_mw = Config_MW::minVal + CP::MWspan * mws;
    const float curr_K = pr.K;

    const float q0 = pr.q0;
    const float p_mix = pr.p_mix;

    // st'(P) = c/(P(1-P)), st''(P) = c^2 (2P-1)/(P(1-P))^2 with c = 0.999998:
    // one reciprocal instead of 1/P and 1/(1-P)
    float P_adj = p_mix * 0.999998f + 0.000001f;
    float ipq = 1.0f / (P_adj * (1.0f - P_adj));
    float d_st_dP = 0.999998f * ipq;
    float d2_st_dP2 = (0.999998f * 0.999998f) * (2.0f * P_adj - 1.0f) * (ipq * ipq);

    // z = K*st(p_mix): its derivatives w.r.t. p_mix
    float dz_dpmix = curr_K * d_st_dP;
    float d2z_dpmix2 = curr_K * d2_st_dP2;

    // --- 1. Update WR0 and WR1 parameters based on current derivatives ---
    float dpmix_dq0 = (1.0f - cur_mw);
    float dz_dq0 = dz_dpmix * dpmix_dq0;
    float d2z_dq02 = d2z_dpmix2 * (dpmix_dq0 * dpmix_dq0);

    const float inv_n_sum = pr.inv_n;   // 1/n_sum from PredictF()
    float inv_n_sum2 = inv_n_sum * inv_n_sum;
    float inv_n_sum3 = inv_n_sum2 * inv_n_sum;

    float dq0_dn0 = n[1] * inv_n_sum2;
    float dq0_dn1 = -n[0] * inv_n_sum2;

    float d2q0_dn02 = -2.0f * n[1] * inv_n_sum3;
    float d2q0_dn12 =  2.0f * n[0] * inv_n_sum3;
    float cross = (n[0] - n[1]) * inv_n_sum3;

    // Chain to the two wr parameters through the traces, then LOGWR: to
    // log coordinates u_j = ln(wr_j), dz/du = dz/dwr * wr and d2z/du2 =
    // d2z/dwr2 * wr^2 + dz/dwr * wr.  The same expression per lane j.
    float dz_dwr[2], d2z_dwr[2];
    for( int j=0; j<2; j++ ) {
      float dq0_dwr  = dq0_dn0 * rt.w[0][j] + dq0_dn1 * rt.w[1][j];
      float d2q0_dwr = d2q0_dn02 * (rt.w[0][j]*rt.w[0][j]) + d2q0_dn12 * (rt.w[1][j]*rt.w[1][j]) + 2.0f * cross * rt.w[0][j] * rt.w[1][j] 
                     + dq0_dn0 * rt.ww[0][j] + dq0_dn1 * rt.ww[1][j];
      float dz  = dz_dq0 * dq0_dwr;
      float d2z = d2z_dq02 * (dq0_dwr * dq0_dwr) + dz_dq0 * d2q0_dwr;
      d2z_dwr[j] = d2z*(cur_wr[j]*cur_wr[j]) + dz*cur_wr[j];
      dz_dwr[j]  = dz * cur_wr[j];
    }

    // UVROT: rotate to u = (u0+u1)/2, v = (u0-u1)/2; diagonal Newton with
    // the shared curvature approximation h_uu ~ h_vv ~ h00 + h11, where
    // h?? = d2pK/dwr?2 / p_t = e_o*(c2*z'^2 + z'') per axis.
    float g_u = dz_dwr[0] + dz_dwr[1];
    float g_v = dz_dwr[0] - dz_dwr[1];
    float hz0 = c2 * (dz_dwr[0] * dz_dwr[0]) + d2z_dwr[0];
    float hz1 = c2 * (dz_dwr[1] * dz_dwr[1]) + d2z_dwr[1];
    float h_d = e_o * (hz0 + hz1);

    // 2.1 UV2X2 + XHESS + RAYCL: one EMA cross term, 2x2 Newton solve with
    // det guard (or P11 CSHRK soft shrink), joint direction-preserving
    // trust region. Reduces to the diagonal path when the guard trips.
    wr0_state.Accum(eeuv * g_u, e_o * g_u, h_d, g);
    wr1_state.Accum(eeuv * g_v, e_o * g_v, h_d, g);

    // XHESS: exact rotated cross diagonal h_uv = h00 - h11 (MIXTR-free part),
    // exposed through the signed shaper XHW with its own clip (G2_u/v are 0
    // in the delivered constants, so the cross channel gets a dedicated one).
    // The cross gradient term is the cell's own (e_o), like the curvature.
    float h_x = clip( e_o * (hz0 - hz1), CP::XHC );
    // The EMA has its own decay, XM2 (M2_x), not the u axis's momentum_R.
    rt.R_uv = rt.R_uv * CP::XM2 + CP::CXW * ((e_o * g_u) * (e_o * g_v) - CP::XHW * h_x) * g;

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
        float agu = 1.0f + CP::AGAu * agduv;
        float agv = 1.0f + CP::AGAv * agduv;
        float r = fmaxf( fmaxf( fabsf(s_u)*CP::iStepU / agu, fabsf(s_v)*CP::iStepV / agv ), 1.0f );
        float ir = r > 1.0f ? 1.0f / r : 1.0f;   // the division only when the clip binds
        wr0_state.Apply(s_u * ir, CP::UVLO, CP::UVHI, Config_U::stepMax * agu);
        wr1_state.Apply(s_v * ir, -CP::UV_VH, CP::UV_VH, Config_V::stepMax * agv);
      } else {
        float agu = 1.0f + CP::AGAu * agduv;
        float agv = 1.0f + CP::AGAv * agduv;
        wr0_state.Apply(wr0_state.StepRaw(), CP::UVLO, CP::UVHI, Config_U::stepMax * agu);
        wr1_state.Apply(wr1_state.StepRaw(), -CP::UV_VH, CP::UV_VH, Config_V::stepMax * agv);
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

    // (The P10 MKCPL coupled (mw,K) variant that used to sit here was dead
    // code -- OPT_MKCPL=0, tested as a loss, see the header notes -- and is
    // not carried into the template.)

    // --- 2. Update MW parameter ---
    float dpmix_dmw = CP::mwP0 - q0;
    float dz_dmw = dz_dpmix * dpmix_dmw;
    float d2z_dmw2 = d2z_dpmix2 * (dpmix_dmw * dpmix_dmw);

    // P24: chain to the logit coordinate, mw = lo + span*sigma(x):
    // f' = span*s(1-s), f'' = f'*(1-2s) (the commonly-dropped term kept).
    {
      float f1m = CP::MWspan * mws * (1.0f - mws);
      float f2m = f1m * (1.0f - 2.0f * mws);
      float g_xm = dz_dmw * f1m;
      float h_xm = d2z_dmw2 * (f1m * f1m) + dz_dmw * f2m;
      mw_state.Update(ee * g_xm, e_o * g_xm, e_o * (c2 * (g_xm * g_xm) + h_xm), CP::MWXLO, CP::MWXHI, g,
                      Config_MW::stepMax * (1.0f + CP::AGAm * agd));
    }

    // --- 3. Update K parameter ---
    const float stP = pr.stP;   // st(p_mix), from PredictF()
    if( fabsf(stP)>=CP::stP_min ) {
      // P24: y = ln K and z = K*st(p_mix), so dz/dy = d2z/dy2 = z
      const float zk = pr.z;
      k_state.Update(ee * zk, e_o * zk, e_o * (c2 * (zk * zk) + zk), CP::KYLO, CP::KYHI, g,
                     Config_K::stepMax * (1.0f + CP::AGAk * agd));
    }


    // --- 4. Update the derivative states for the NEXT cycle using NEW parameter weights ---
    cur_wr[0] = expf(clamp(wr0_state.val + wr1_state.val, CP::UVLO, CP::UVHI));
    cur_wr[1] = expf(clamp(wr0_state.val - wr1_state.val, CP::UVLO, CP::UVHI));
    
    // The retention weights (1 - decay_rate) for an observation of weight
    // g: the count of the symbol that occurred (n[bit]) decays by wr0's,
    // the other by wr1's, and every trace of n_i decays with n_i.
    float w0_retention = 1.0f - cur_wr[0]*g;
    float w1_retention = 1.0f - cur_wr[1]*g;

    const float ret[2] = { (bit == 0) ? w0_retention : w1_retention,
                           (bit == 0) ? w1_retention : w0_retention };

    // Applying the chain rule for d/d(alpha).  Since (1 - alpha) handles
    // retention, the injected variable acts negatively in the derivative,
    // d/d_alpha (1 - alpha)*n = -n: n_i's trace w.r.t. the wr that decayed
    // it this step (wr0 for n[bit], wr1 for the other, i.e. lane j = bit^i)
    // gets -n_i*g, its second-order trace -2*w*g.  Every lane is the same
    // multiply-subtract, with a 0 injected on the six lanes not selected --
    // x*r - 0 is exactly x*r, so this is the old per-lane code bit for bit.
    // The selection first, into lane arrays, so that the update below is a
    // plain elementwise loop the compiler vectorizes.
    const int sel[2] = { bit, bit ^ 1 };
    float rn[2][2], dw[2][2], dww[2][2];
    for( int i=0; i<2; i++ ) for( int j=0; j<2; j++ ) {
      const bool inj = ( j == sel[i] );
      rn[i][j]  = ret[i];
      dw[i][j]  = inj ? n[i] * g : 0.0f;
      dww[i][j] = inj ? 2.0f * rt.w[i][j] * g : 0.0f;
    }
    for( int i=0; i<2; i++ ) for( int j=0; j<2; j++ ) {
      rt.ww[i][j] = rt.ww[i][j] * rn[i][j] - dww[i][j];
      rt.w[i][j]  = rt.w[i][j]  * rn[i][j] - dw[i][j];
    }

    // --- 5. Update n0 and n1 model values ---
    const float cnt[2] = { (1.0f - float(bit)) * g, float(bit) * g };
    for( int i=0; i<2; i++ ) n[i] = n[i] * ret[i] + cnt[i];

    // the RTRL leaks (clamped to [0,1] in the bundle)
    for( int i=0; i<2; i++ ) for( int j=0; j<2; j++ ) { rt.w[i][j] *= CP::leakage1; rt.ww[i][j] *= CP::leakage2; }
  }

}; // struct
