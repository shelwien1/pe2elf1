
// -------------------------------------------------------------
// sh_mix2.inc -- 2-input logistic mixer with Newton weight updates.
//
//   p = sq( w*st(p1) + (1-w)*st(p2) + b ),   w = sq(W)
//
// per mixer context (IDX Index of sh_model-M0.idx): W (logit coordinate,
// box +-Wclip) and the stretch-domain bias b (box +-Bclip) are
// ParamUpdater states -- the diagonal Newton step of newton.inc's
// BinaryMixer, with the same first/second-derivative chain, here for two
// arbitrary probabilities instead of the two model trees.  Frozen
// (NW = 0, b = 0) it is the SSE stage's static stretch blend with
// w = 1 - W_sse; adaptive it learns the blend per context.
// -------------------------------------------------------------

template<class CP> struct Mix2 {
  struct Cell {
    ParamUpdater<1, typename CP::Config_W> w;
    ParamUpdater<1, typename CP::Config_B> b;
  };
  Cell* t;
  uint  n;

  // per-query state, valid between Mix() and Update()
  Cell* c;
  float s1, s2, wv, p0;

  static float stf( float p ) {
    p = clamp( p, CP::Pmin, 1.0f-CP::Pmin );
    return logf( p/(1.0f-p) );
  }
  static float sqf( float x ) { return 1.0f/(1.0f+expf(-x)); }

  void Init( uint contexts ) {
    n = contexts;
    t = new Cell[n];
    float w0 = clamp( CP::W0, 1.0f/65536, 1.0f-1.0f/65536 );
    float W0 = clip( logf(w0/(1.0f-w0)), CP::Wclip );
    for( uint i=0; i<n; i++ ) { t[i].w.Init(W0); t[i].b.Init(0.0f); }
  }
  void Quit() { delete[] t; t=0; }

  // p1, p2 = P(bit==0) of the two inputs; returns the mixed P(bit==0)
  float Mix( uint cx, float p1, float p2 ) {
    c  = t + (cx < n ? cx : cx % n);
    s1 = stf(p1);
    s2 = stf(p2);
    wv = sqf( c->w.val );
    float st_mix = wv*s1 + (1.0f-wv)*s2 + (CP::BIAS ? c->b.val : 0.0f);
    p0 = sqf( st_mix );
    return p0;
  }

  void Update( int bit ) {
    float pK_t = bit ? (1.0f - p0) : p0;
    float sign = bit ? -1.0f : 1.0f;
    float inv_p  = 1.0f / fmaxf( pK_t, 1e-12f );
    float inv_p2 = inv_p * inv_p;

    // dp0/dst = p0(1-p0), d2p0/dst2 = p0(1-p0)(1-2p0)
    float p0_1 = p0 * (1.0f - p0);
    float p0_2 = p0_1 * (1.0f - 2.0f * p0);

    // weight: st = w*s1 + (1-w)*s2, w = sq(W)
    float diff = s1 - s2;
    float w_1  = wv * (1.0f - wv);
    float w_2  = w_1 * (1.0f - 2.0f * wv);
    float dp0_dw   = p0_1 * diff;
    float d2p0_dw2 = p0_2 * (diff * diff);
    float dp0_dW   = dp0_dw * w_1;
    float d2p0_dW2 = d2p0_dw2 * (w_1 * w_1) + dp0_dw * w_2;
    c->w.Update( dp0_dW * sign, d2p0_dW2 * sign, inv_p, inv_p2, 0.0f, 1.0f, -CP::Wclip, CP::Wclip );

    // bias: dp0/db = p0(1-p0), d2p0/db2 = p0(1-p0)(1-2p0)
    if( CP::BIAS )
      c->b.Update( p0_1 * sign, p0_2 * sign, inv_p, inv_p2, 0.0f, 1.0f, -CP::Bclip, CP::Bclip );
  }
};
