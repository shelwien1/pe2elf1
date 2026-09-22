
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

  // gd = the descent gradient -dL/dtheta accumulated into D; 
  // gr = the cell's own -dL/dtheta and hc = d2p/dtheta2 / p_t, so that gr^2 - hc is
  // the exact d2L/dtheta2 of the cell's -ln p, accumulated into R 
  // (the callers form all three in the logit domain, see Counter::C_Update).
  // gd differs from gr only under A2, where the gradient is blended with
  // the final coder's error while the curvature stays the cell's own.
  // w = weight of this observation (1 = a full event), scaling the loss.
  // Returns the weighted descent gradient (for the Nesterov look-ahead).
  INLINE float Accum( float gd, float gr, float hc, float w = 1.0f ) {
    GRAD_HOOK( gd, gr, hc );
    float dp_inv = gd * w;
    float dp2_inv = (gr * gr) * w;
    float d2p_inv = hc * w;

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

  void Update( float gd, float gr, float hc, float lo, float hi, float w, float smax ) {
    float dp_inv = Accum(gd, gr, hc, w);
    if( cfg::NAG ) {
      // Nesterov look-ahead: the step from momentum_D*D_new - g instead of D_new
      float Dn = D * cfg::momentum_D - dp_inv;
      Apply( cfg::NW * Dn / (R + cfg::inc), lo, hi, smax );
    } else Apply(StepRaw(), lo, hi, smax);
  }
  // the plain case: descent gradient = own gradient
  void Update( float gr, float hc, float lo, float hi, float w = 1.0f, float smax = cfg::stepMax ) {
    Update( gr, gr, hc, lo, hi, w, smax );
  }

};

// Inactive parameter state
template<class cfg> struct ParamUpdater<0, cfg> {
  void Init(float /*init_val*/) {}
  inline float Accum(float, float, float, float = 1.0f) { return 0.0f; }
  inline float Denom() const { return 1.0f; }
  inline float StepRaw() const { return 0.0f; }
  inline void Apply(float, float, float, float = 0.0f) {}
  inline void Update(float /*gd*/, float /*gr*/, float /*hc*/, float /*lo*/, float /*hi*/, float, float) {}
  inline void Update(float /*gr*/, float /*hc*/, float /*lo*/, float /*hi*/, float = 1.0f, float = 0.0f) {}
};
