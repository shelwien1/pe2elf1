
// -------------------------------------------------------------
// sh_mix2.inc -- 2-input logistic mixer with Newton weight updates.
//
//   p = sq( w*st(p1) + (1-w)*st(p2) + b ),   w = f(W)
//
// per mixer context (IDX Index of sh_model-M0.idx): the weight parameter
// W and the stretch-domain bias b (box +-Bclip) are ParamUpdater states --
// the diagonal Newton step of newton.inc's BinaryMixer, with the same
// first/second-derivative chain, here for two arbitrary probabilities
// instead of the two model trees.  WDOM selects the weight's domain f:
//   0  logistic   w = sq(W),  W in +-Wclip     (newton.inc)
//   1  linear     w = W,      w in [-Wlin, 1+Wlin]
//   2  log        w = exp(W), W in +-Wclip
//   3  reciprocal w = 1/V,    V in [1/(1+Wlin), 64]
//   4  free       st = W1*st(p1) + W2*st(p2) + b, W1,W2 in [-Wlin, 1+Wlin]
// Frozen (NW = 0, b = 0) it is the SSE stage's static stretch blend with
// w = 1 - W_sse; adaptive it learns the blend per context.
// -------------------------------------------------------------

#ifndef MIX_MAXCTX_LOG
#define MIX_MAXCTX_LOG 24      // cap on mixer contexts; beyond it they are hashed
#endif

template<class CP> struct Mix2 {
  struct Cell {
    ParamUpdater<1, typename CP::Config_W> w;    // W (or W1)
    ParamUpdater<1, typename CP::Config_W> w2;   // W2, WDOM 4 only
    ParamUpdater<1, typename CP::Config_B> b;
  };
  Cell* t;
  uint  n;        // contexts allocated
  uint  vol;      // contexts asked for; hashed when vol > n

  int   dom;      // clamped CP::WDOM
  float lo, hi;   // box of the weight parameter(s)

  // per-query state, valid between Mix() and Update()
  Cell* c;
  float s1, s2, wv, wv2, p0;

  static float stf( float p ) {
    p = clamp( p, CP::Pmin, 1.0f-CP::Pmin );
    return logf( p/(1.0f-p) );
  }
  static float sqf( float x ) { return 1.0f/(1.0f+expf(-x)); }

  void Init( uint contexts ) {
    vol = contexts < 1 ? 1 : contexts;
    n = vol < (1u<<MIX_MAXCTX_LOG) ? vol : (1u<<MIX_MAXCTX_LOG);
    t = new Cell[n];
    dom = CP::WDOM<0 ? 0 : CP::WDOM>4 ? 4 : CP::WDOM;
    float w0 = clamp( CP::W0, 1.0f/65536, 1.0f-1.0f/65536 );
    float wl = clamp( CP::Wlin, 0.0f, 16.0f );
    float W0, W20 = 0.0f;
    switch( dom ) {
      default: lo = -CP::Wclip; hi = CP::Wclip;  W0 = clamp( logf(w0/(1.0f-w0)), lo, hi ); break;
      case 1:  lo = -wl; hi = 1.0f+wl;            W0 = w0; break;
      case 2:  lo = -CP::Wclip; hi = CP::Wclip;  W0 = clamp( logf(w0), lo, hi ); break;
      case 3:  lo = 1.0f/(1.0f+wl); hi = 64.0f;   W0 = clamp( 1.0f/w0, lo, hi ); break;
      case 4:  lo = -wl; hi = 1.0f+wl;            W0 = w0; W20 = 1.0f-w0; break;
    }
    for( uint i=0; i<n; i++ ) { t[i].w.Init(W0); t[i].w2.Init(W20); t[i].b.Init(0.0f); }
  }
  void Quit() { delete[] t; t=0; }

  // p1, p2 = P(bit==0) of the two inputs; returns the mixed P(bit==0)
  float Mix( uint cx, float p1, float p2 ) {
    if( !CP::ON ) { p0 = p2; return p2; }     // bypass: the SSE output as is
    c  = t + ( vol<=n ? cx : uint( (qword(cx*0x9E3779B1u) * n) >> 32 ) );
    s1 = stf(p1);
    s2 = stf(p2);
    float W = c->w.val;
    switch( dom ) {
      default: wv = sqf(W);            wv2 = 1.0f-wv;  break;
      case 1:  wv = W;                 wv2 = 1.0f-wv;  break;
      case 2:  wv = expf(W);           wv2 = 1.0f-wv;  break;
      case 3:  wv = 1.0f/W;            wv2 = 1.0f-wv;  break;
      case 4:  wv = W;                 wv2 = c->w2.val; break;
    }
    float st_mix = wv*s1 + wv2*s2 + (CP::BIAS ? c->b.val : 0.0f);
    p0 = sqf( st_mix );
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

    if( dom==4 ) {
      // free weights: dp/dW1 = p'(st) s1, d2p/dW1^2 = p''(st) s1^2 (same for W2, s2)
      c->w.Update ( p0_1*s1 * sign, p0_2*(s1*s1) * sign, inv_p, inv_p2, 0.0f, 1.0f, lo, hi );
      c->w2.Update( p0_1*s2 * sign, p0_2*(s2*s2) * sign, inv_p, inv_p2, 0.0f, 1.0f, lo, hi );
    } else {
      // convex weight: st = w*s1 + (1-w)*s2, w = f(W); f', f'' per domain
      float diff = s1 - s2;
      float w_1, w_2;
      switch( dom ) {
        default: w_1 = wv * (1.0f - wv); w_2 = w_1 * (1.0f - 2.0f * wv); break;   // logistic
        case 1:  w_1 = 1.0f;             w_2 = 0.0f;                     break;   // linear
        case 2:  w_1 = wv;               w_2 = wv;                       break;   // exp
        case 3:  w_1 = -wv*wv;           w_2 = 2.0f*wv*wv*wv;            break;   // 1/V
      }
      float dp0_dw   = p0_1 * diff;
      float d2p0_dw2 = p0_2 * (diff * diff);
      float dp0_dW   = dp0_dw * w_1;
      float d2p0_dW2 = d2p0_dw2 * (w_1 * w_1) + dp0_dw * w_2;
      c->w.Update( dp0_dW * sign, d2p0_dW2 * sign, inv_p, inv_p2, 0.0f, 1.0f, lo, hi );
    }

    // bias: dp0/db = p0(1-p0), d2p0/db2 = p0(1-p0)(1-2p0)
    if( CP::BIAS )
      c->b.Update( p0_1 * sign, p0_2 * sign, inv_p, inv_p2, 0.0f, 1.0f, -CP::Bclip, CP::Bclip );
  }
};
