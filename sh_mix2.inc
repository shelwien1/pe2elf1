
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
    float rx;   // EMA of the (W, b) cross curvature, X2 only
  };
  Cell* t;        // the Table() of the generated M0_T
  uint  n;        // contexts
  uint  vol;      // contexts asked for; hashed when vol > n

  // per-query state, valid between Mix() and Update()
  Cell* c;
  float s1, s2, wv, p0;

  static float stf( float p ) {
    p = clamp( p, CP::Pmin, 1.0f-CP::Pmin );
    return logf( p/(1.0f-p) );
  }
  static float sqf( float x ) { return 1.0f/(1.0f+expf(-x)); }

  // table: mix_rows(contexts) cells, owned by the caller
  void Init( Cell* table, uint contexts ) {
    t = table; n = 0;
    if( !CP::ON ) return;
    vol = contexts < 1 ? 1 : contexts;
    n = mix_rows( vol );
    float w0 = clamp( CP::W0, 1.0f/65536, 1.0f-1.0f/65536 );
    float W0 = clip( rt_logf(w0/(1.0f-w0)), CP::Wclip );
    for( uint i=0; i<n; i++ ) { t[i].w.Init(W0); t[i].b.Init(0.0f); t[i].rx = 0.0f; }
  }
  // p1, p2 = P(bit==0) of the two inputs; returns the mixed P(bit==0)
  float Mix( uint cx, float p1, float p2 ) {
    if( !CP::ON ) return p2;
    c  = t + ( vol<=n ? cx : uint( (qword(cx*0x9E3779B1u) * n) >> 32 ) );
    s1 = stf(p1);
    s2 = stf(p2);
    wv = sqf( c->w.val );
    p0 = sqf( wv*s1 + (1.0f-wv)*s2 + c->b.val );
    return p0;
  }

  void Update( int bit ) {
    if( !CP::ON ) return;
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

    if( CP::X2 ) {
      // coupled (W, b) Newton step: EMA of the cross curvature of -ln p,
      // gW*gb/p^2 - (d2p/dWdb)/p with d2p/dWdb = p''(st)*(s1-s2)*w(1-w), then
      // the det-guarded 2x2 solve with joint ray clipping (as the counters'
      // (u, v) pair)
      typedef typename CP::Config_W CW; typedef typename CP::Config_B CB;
      float gW = dp0_dW * sign, gB = p0_1 * sign;
      c->w.Accum( gW, d2p0_dW2 * sign, inv_p, inv_p2, 0.0f );
      c->b.Accum( gB, p0_2 * sign,     inv_p, inv_p2, 0.0f );
      float hx = sign * p0_2 * diff * w_1;
      c->rx = c->rx * CW::momentum_R + CP::CXW * (gW * gB * inv_p2 - hx * inv_p);
      float a_w = c->w.Denom(), a_b = c->b.Denom(), cc = c->rx;
      float det = a_w * a_b - cc * cc;
      if( det > CP::DET * a_w * a_b ) {
        float b_w = CW::NW * c->w.D, b_b = CB::NW * c->b.D;
        float idet = 1.0f / det;
        float s_w = (b_w * a_b - cc * b_b) * idet;
        float s_b = (a_w * b_b - cc * b_w) * idet;
        float r = fmaxf( fmaxf( fabsf(s_w) / CW::stepMax, fabsf(s_b) / CB::stepMax ), 1.0f );
        float ir = 1.0f / r;
        c->w.Apply( s_w * ir, -CP::Wclip, CP::Wclip );
        c->b.Apply( s_b * ir, -CP::Bclip, CP::Bclip );
      } else {
        c->w.Apply( c->w.StepRaw(), -CP::Wclip, CP::Wclip );
        c->b.Apply( c->b.StepRaw(), -CP::Bclip, CP::Bclip );
      }
      return;
    }

    c->w.Update( dp0_dW * sign, d2p0_dW2 * sign, inv_p, inv_p2, 0.0f, 1.0f, -CP::Wclip, CP::Wclip );

    // bias: dp0/db = p0(1-p0), d2p0/db2 = p0(1-p0)(1-2p0)
    c->b.Update( p0_1 * sign, p0_2 * sign, inv_p, inv_p2, 0.0f, 1.0f, -CP::Bclip, CP::Bclip );
  }
};
