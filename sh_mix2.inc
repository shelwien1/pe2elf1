
// -------------------------------------------------------------
// sh_mix2.inc -- 2-input logistic mixer with Newton weight updates.
//
//   p = sq( w*z1 + (1-w)*z2 + b ),   w = sq(W),   z = st(p) of the inputs
//
// per mixer context (IDX Index of sh_model-M0.idx): W (logit coordinate,
// box +-Wclip) and the stretch-domain bias b (box +-Bclip) are
// ParamUpdater states -- the diagonal Newton step of newton.inc's
// BinaryMixer (with a Nesterov look-ahead, NAGw/NAGb), the same first and
// second derivative chain, here for two arbitrary probabilities instead
// of the two model trees.  Frozen
// (NW = 0, b = 0) it is a static stretch-domain blend; adaptive it learns
// the blend per context.  ON=0 bypasses it: the final p is p2.
// -------------------------------------------------------------

#ifndef MIX_MAXCTX_LOG
#define MIX_MAXCTX_LOG 24      // cap on mixer contexts; beyond it they are hashed
#endif

// Contexts of the mixer table for a context volume (sizes the Table() of
// the generated M0_T in the shipping build).
static constexpr uint mix_rows( qword vol ) {
  return uint( vol < 1 ? 1 : vol < (qword(1)<<MIX_MAXCTX_LOG) ? vol : (qword(1)<<MIX_MAXCTX_LOG) );
}

template<class CP> struct Mix2 {
  struct Cell {
    ParamUpdater<1, typename CP::Config_W> w;
    ParamUpdater<1, typename CP::Config_B> b;
    uint age;   // updates seen (saturating): the young-cell step schedule (A1)
  };
  Cell* t;        // the Table() of the generated M0_T
  uint  n;        // contexts
  uint  vol;      // contexts asked for; hashed when vol > n

  // per-query state, valid between Mix() and Update()
  Cell* c;
  float s1, s2, wv, p0;
  float d1, d2;   // dz_final/dz1, dz_final/dz2 of the last Mix(): the weight, or 0 where the input clip binds
  float zm;       // |logit| bound of the inputs, = the input clip [Pmin, 1-Pmin]

  static float sqf( float x ) { return 1.0f/(1.0f+expf(-x)); }

  // table: mix_rows(contexts) cells, owned by the caller
  void Init( Cell* table, uint contexts ) {
    t = table; n = 0;
    if( !CP::ON ) return;
    vol = contexts < 1 ? 1 : contexts;
    n = mix_rows( vol );
    zm = rt_logf( (1.0f - CP::Pmin) / CP::Pmin );
    float w0 = clamp( CP::W0, 1.0f/65536, 1.0f-1.0f/65536 );
    float W0 = clip( rt_logf(w0/(1.0f-w0)), CP::Wclip );
    for( uint i=0; i<n; i++ ) { t[i].w.Init(W0); t[i].b.Init(0.0f); t[i].age = 0; }
  }
  // z1, z2 = logits of P(bit==0) of the two inputs; returns the mixed P(bit==0)
  // derivatives of the final logit w.r.t. the two inputs of the last Mix()
  // (the A2 chain): w and 1-w, or 0 where the input clip binds; with the
  // mixer off the final p is sq(z2): 0 and 1.
  float dzf_dz1() const { return CP::ON ? d1 : 0.0f; }
  float dzf_dz2() const { return CP::ON ? d2 : 1.0f; }

  float Mix( uint cx, float z1, float z2 ) {
    if( !CP::ON ) return sqf(z2);
    c  = t + ( vol<=n ? cx : uint( (qword(cx*0x9E3779B1u) * n) >> 32 ) );
    s1 = clamp( z1, -zm, zm );
    s2 = clamp( z2, -zm, zm );
    wv = sqf( c->w.val );
    d1 = ( z1 > -zm && z1 < zm ) ? wv : 0.0f;
    d2 = ( z2 > -zm && z2 < zm ) ? 1.0f - wv : 0.0f;
    p0 = sqf( wv*s1 + (1.0f-wv)*s2 + c->b.val );
    return p0;
  }

  void Update( int bit ) {
    if( !CP::ON ) return;
    // logit-domain chain (see Counter::C_Update): with z = w*s1+(1-w)*s2+b
    // and p0 = sq(z), the gradient of -ln p w.r.t. a parameter t is
    // e_o*dz/dt and the curvature term e_o*((1-2p0)*(dz/dt)^2 + d2z/dt2),
    // e_o = sign*p_o, p_o = the other symbol's probability; no reciprocal.
    float p_o  = bit ? p0 : (1.0f - p0);
    float sign = bit ? -1.0f : 1.0f;
    float e_o  = sign * p_o;
    float c2   = 1.0f - 2.0f * p0;

    // A1: young-cell step schedule, stepMax*(1 + AGA/(1 + age/B)); AGA = 0
    // reproduces the plain limit bit for bit.
    float agd = 1.0f / (1.0f + float(c->age) * CP::AGiB);
    if( c->age != ~0u ) c->age++;

    // weight: z = w*s1 + (1-w)*s2 + b, w = sq(W): dz/dW = (s1-s2) w(1-w),
    // d2z/dW2 = (s1-s2) w(1-w)(1-2w)
    float diff = s1 - s2;
    float w_1  = wv * (1.0f - wv);
    float w_2  = w_1 * (1.0f - 2.0f * wv);
    float dz_dW   = diff * w_1;
    float d2z_dW2 = diff * w_2;

    c->w.Update( e_o * dz_dW, e_o * (c2 * (dz_dW * dz_dW) + d2z_dW2), -CP::Wclip, CP::Wclip, 1.0f,
                 CP::Config_W::stepMax * (1.0f + CP::AGAw * agd) );

    // bias: dz/db = 1, d2z/db2 = 0
    c->b.Update( e_o, e_o * c2, -CP::Bclip, CP::Bclip, 1.0f,
                 CP::Config_B::stepMax * (1.0f + CP::AGAb * agd) );
  }
};
