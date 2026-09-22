
// -------------------------------------------------------------
// sh_mix2.inc -- 2-input logistic mixer with Newton weight updates.
//
//   p = sq( w*z1 + (1-w)*z2 + b ),   w = sq(W),   z = st(p) of the inputs
//
// One Mix2<CP> is one mixer context (IDX Index of sh_model-M0.idx): W
// (logit coordinate, box +-Wclip) and the stretch-domain bias b (box
// +-Bclip) are ParamUpdater states -- the diagonal Newton step of
// newton.inc's BinaryMixer (with a Nesterov look-ahead, NAGw/NAGb), the
// same first and second derivative chain, here for two arbitrary
// probabilities instead of the two model trees.  Frozen (NW = 0, b = 0)
// it is a static stretch-domain blend; adaptive it learns the blend per
// context.
//
// Like Counter, the cell holds model state only: Mix() returns what
// Update() needs in a Pred the caller keeps, and the table of cells is the
// Table() of the generated M0_T, indexed by the caller.
// -------------------------------------------------------------

template<class CP> struct Mix2 {
  ParamUpdater<typename CP::Config_W> w;   // logit of the weight of z1
  ParamUpdater<typename CP::Config_B> b;   // stretch-domain bias
  uint age;   // updates seen (saturating): the young-cell step schedule (A1)

  // What Mix() computes and Update() needs back: per-query state, kept by
  // the caller between the two calls
  struct Pred {
    float p;        // the mixed P(bit==0)
    float s1, s2;   // the two inputs after the +-ZM clip
    float wv;       // sq(w.val), the weight of z1
    float d1, d2;   // dz_final/dz1, dz_final/dz2 (the A2 chain): wv and 1-wv, or 0 where the input clip binds
  };

  static float sqf( float x ) { return 1.0f/(1.0f+expf(-x)); }

  void Init() { w.Init(CP::WX0); b.Init(0.0f); age = 0; }

  // z1, z2 = logits of P(bit==0) of the two inputs
  Pred Mix( float z1, float z2 ) const {
    Pred r;
    r.s1 = clamp( z1, -CP::ZM, CP::ZM );
    r.s2 = clamp( z2, -CP::ZM, CP::ZM );
    r.wv = sqf( w.val );
    r.d1 = ( z1 > -CP::ZM && z1 < CP::ZM ) ? r.wv : 0.0f;
    r.d2 = ( z2 > -CP::ZM && z2 < CP::ZM ) ? 1.0f - r.wv : 0.0f;
    r.p  = sqf( r.wv*r.s1 + (1.0f-r.wv)*r.s2 + b.val );
    return r;
  }

  void Update( int bit, const Pred& pr ) {
    // logit-domain chain (see Counter::C_Update): with z = w*s1+(1-w)*s2+b
    // and p = sq(z), the gradient of -ln p w.r.t. a parameter t is
    // e_o*dz/dt and the curvature term e_o*((1-2p)*(dz/dt)^2 + d2z/dt2),
    // e_o = sign*p_o, p_o = the other symbol's probability; no reciprocal.
    float p_o  = bit ? pr.p : (1.0f - pr.p);
    float sign = bit ? -1.0f : 1.0f;
    float e_o  = sign * p_o;
    float c2   = 1.0f - 2.0f * pr.p;

    // A1: young-cell step schedule, stepMax*(1 + AGA/(1 + age/B)); AGA = 0
    // reproduces the plain limit bit for bit.
    float agd = 1.0f / (1.0f + float(age) * CP::AGiB);
    if( age != ~0u ) age++;

    // weight: z = w*s1 + (1-w)*s2 + b, w = sq(W): dz/dW = (s1-s2) w(1-w),
    // d2z/dW2 = (s1-s2) w(1-w)(1-2w)
    float diff = pr.s1 - pr.s2;
    float w_1  = pr.wv * (1.0f - pr.wv);
    float w_2  = w_1 * (1.0f - 2.0f * pr.wv);
    float dz_dW   = diff * w_1;
    float d2z_dW2 = diff * w_2;

    w.Update( e_o * dz_dW, e_o * (c2 * (dz_dW * dz_dW) + d2z_dW2), -CP::Wclip, CP::Wclip, 1.0f,
              CP::Config_W::stepMax * (1.0f + CP::AGAw * agd) );

    // bias: dz/db = 1, d2z/db2 = 0
    b.Update( e_o, e_o * c2, -CP::Bclip, CP::Bclip, 1.0f,
              CP::Config_B::stepMax * (1.0f + CP::AGAb * agd) );
  }
};
