
// -------------------------------------------------------------
// sh_mixN.inc -- N-input logistic mixer (N >= 2) with Newton weight updates.
//
//   p = sq( sum_i w_i*s_i + b ),   s_i = clip(z_i, +-ZM),   z_i = logits of the inputs
//
// The weights are a softmax with the last input as the reference: N-1 free
// logits W_k (k < N-1), the reference's pinned at 0,
//
//   w_k     = e^W_k / (1 + sum_{j<N-1} e^W_j)
//           = 1 / (1 + e^-W_k + sum_{j<N-1, j!=k} e^(W_j-W_k))      k < N-1
//   w_{N-1} = 1 - sum_{k<N-1} w_k
//
// so they stay convex (sum 1, each in (0,1)), and there are exactly N-1 of
// them: a free logit for the reference too would leave a direction that
// moves no output (adding a constant to every W), along which the steps
// drift into the box walls.  At N = 2 this is Mix2 (sh_mix2.inc): W_0 is the
// logit of the weight of z_0 against z_1, w_0 = sq(W_0).
//
// Each W_k and the stretch-domain bias b is a ParamUpdater state -- the same
// bundle as Mix2 (config_mix2.hpp: Config_W shared by all N-1 weight logits,
// Config_B, the boxes +-Wclip / +-Bclip, the A1 age schedule, the A2 E2E
// blend) plus the one knob of N > 2, XW.  The chain, with zbar = sum_i w_i s_i
// and g_k = dz/dW_k:
//
//   dz/dW_k       = w_k (s_k - zbar) = sum_{i!=k} w_k w_i (s_k - s_i)
//   d2z/dW_k^2    = g_k (1 - 2 w_k)
//   d2z/dW_j dW_k = -(w_j g_k + w_k g_j)           j != k
//   dz/ds_i       = w_i                           (d[i]; 0 where the input clip binds)
//
// The step.  Each W_k keeps its ParamUpdater D (gradient EMA) and R
// (curvature EMA), and next to them the cell keeps an EMA X_jk of every
// cross curvature d2L/dW_j dW_k (same decay, clip and box as R).  The step
// on W_k is
//
//   NW*D_k / (R_k + inc + XW * sum_{j!=k} |X_jk|)
//
// -- at XW = 1 the diagonal of Gershgorin's bound, a diagonal matrix >= the
// curvature matrix A (A_kk = R_k + inc, A_jk = X_jk), so the step never
// overshoots the quadratic model along any direction.  The weight logits
// are strongly coupled: the inputs of a mixer all predict the same bit, and
// every W_k moves the same zbar.  Plain diagonal steps (XW = 0) then add up to
// an overshoot that grows with N (two fully coupled logits: 2x); with Mix2's
// tuned NW, 3 inputs lost 0.1% that way, and wanted NW at 0.35x.  The full
// solve A s = NW*D is worse still: A is near-singular along "all logits
// together" whenever the inputs agree, and the solve amplifies whatever of D
// lies there (+0.2% at 3 inputs).  The bound takes neither extreme.
//
// At N = 2 there is one logit, no cross term and no bound: every float
// operation is Mix2's, in Mix2's order, and MixN<CP,2> reproduces Mix2's
// stream byte for byte.  The pairwise forms (the weights by 1/(1 + sum of
// exps), the gradient as a sum over pairs) cost O(N^2) instead of O(N) for
// that, which is nothing at the N a coder mixes.
//
// Seeds: W_0 = WX0 (the W0 knob), the other logits 0 -- input 0 starts at
// weight W0 against each other input, and those start level with each other.
// At N = 2, W0 is the initial weight of z_0, as in Mix2.
//
// Like Mix2, the cell holds model state only: Mix() returns what Update()
// needs in a Pred the caller keeps, and the table of cells is the Table() of
// the generated M0_T, indexed by the caller.
// -------------------------------------------------------------

// the cross-curvature EMAs of the weight logits, X_jk for j < k in row
// order: none at N = 2 (an empty base, so MixN<CP,2> is Mix2's size)
template<int NX> struct MixN_X { float X[NX]; void InitX() { for( int q=0; q<NX; q++ ) X[q] = 0.0f; } };
template<> struct MixN_X<0> { void InitX() {} };

template<class CP, int N> struct MixN : MixN_X<(N-1)*(N-2)/2> {
  static_assert( N >= 2, "MixN: at least two inputs" );
  static const int M = N-1;   // weight logits

  ParamUpdater<typename CP::Config_W> W[M];   // weight logits of z_0..z_{N-2}, against z_{N-1}
  ParamUpdater<typename CP::Config_B> b;      // stretch-domain bias
  uint age;   // updates seen (saturating): the young-cell step schedule (A1)

  // What Mix() computes and Update() needs back: per-query state, kept by
  // the caller between the two calls
  struct Pred {
    float z;        // the mixed logit sum_i w_i*s_i + b: what the next stage takes
    float p;        // sq(z), the mixed P(bit==0): the mixer's own prediction
    float s[N];     // the inputs after the +-ZM clip
    float w[N];     // their weights
    float d[N];     // dz/dz_i (the A2 chain): w[i], or 0 where input i's clip binds
  };

  static float sqf( float x ) { return 1.0f/(1.0f+expf(-x)); }

  void Init() {
    W[0].Init( CP::WX0 );
    for( int k=1; k<M; k++ ) W[k].Init( 0.0f );
    b.Init( 0.0f );
    age = 0;
    this->InitX();
  }

  // z[i] = logit of P(bit==0) of input i
  Pred Mix( const float (&z)[N] ) const {
    Pred r;
    for( int i=0; i<N; i++ ) r.s[i] = clamp( z[i], -CP::ZM, CP::ZM );

    // the weights; the reference's is what the others leave
    float wl = 1.0f;
    for( int k=0; k<M; k++ ) {
      float den = 1.0f + expf( -W[k].val );
      for( int j=0; j<M; j++ ) if( j != k ) den += expf( W[j].val - W[k].val );
      r.w[k] = 1.0f / den;
      wl -= r.w[k];
    }
    r.w[M] = wl;

    for( int i=0; i<N; i++ ) r.d[i] = ( z[i] > -CP::ZM && z[i] < CP::ZM ) ? r.w[i] : 0.0f;

    float zm = r.w[0]*r.s[0];
    for( int i=1; i<N; i++ ) zm += r.w[i]*r.s[i];
    r.z = zm + b.val;
    r.p = sqf( r.z );
    return r;
  }

  // ef = A2 end-to-end error: dL_final/dz where z is this cell's mixed
  // logit, i.e. (p_final - [bit==0]) * dz_final/dz -- the same convention as
  // Counter::C_Update and Mix2::Update.  The gradient uses (1-E2E)*own +
  // E2E*end-to-end error, the curvature stays the mixer's own.
  void Update( int bit, const Pred& pr, float ef ) {
    typedef typename CP::Config_W CW;
    // logit-domain chain (see Counter::C_Update): the gradient of -ln p
    // w.r.t. a parameter t is e_o*dz/dt and the curvature term
    // e_o*((1-2p)*(dz/dt)^2 + d2z/dt2), e_o = sign*p_o, p_o = the other
    // symbol's probability.
    float p_o  = bit ? pr.p : (1.0f - pr.p);
    float sign = bit ? -1.0f : 1.0f;
    float e_o  = sign * p_o;
    float ee   = (1.0f - CP::E2E) * e_o - CP::E2E * ef;   // the descent error, -dL/dz
    float c2   = 1.0f - 2.0f * pr.p;

    // A1: young-cell step schedule, stepMax*(1 + AGA/(1 + age/B)); AGA = 0
    // reproduces the plain limit bit for bit.
    float agd = 1.0f / (1.0f + float(age) * CP::AGiB);
    if( age != ~0u ) age++;

    // weight logits: dz/dW_k = sum_{i!=k} (s_k-s_i) w_k w_i,
    // d2z/dW_k^2 = sum_{i!=k} (s_k-s_i) w_k w_i (1-2w_k).  Each W_k's own D and
    // R first (Accum), then the steps, which at N > 2 see all of them.
    float g[M], rhs[M];
    for( int k=0; k<M; k++ ) {
      const float ck = 1.0f - 2.0f * pr.w[k];
      float dz_dW = 0.0f, d2z_dW2 = 0.0f;
      bool first = true;
      for( int i=0; i<N; i++ ) if( i != k ) {
        float dd  = pr.s[k] - pr.s[i];
        float wki = pr.w[k] * pr.w[i];
        float t1  = dd * wki;
        float t2  = dd * (wki * ck);
        if( first ) { dz_dW = t1; d2z_dW2 = t2; first = false; }   // no 0+x: -0 stays -0, as in Mix2
        else { dz_dW += t1; d2z_dW2 += t2; }
      }
      g[k] = dz_dW;
      float dpi = W[k].Accum( ee * dz_dW, e_o * dz_dW, e_o * (c2 * (dz_dW * dz_dW) + d2z_dW2) );
      // the numerator, NW*D -- or NW*(momentum_D*D - g), the Nesterov
      // look-ahead -- as ParamUpdater::Update forms it
      rhs[k] = CW::NAG ? CW::NW * (W[k].D * CW::momentum_D - dpi) : CW::NW * W[k].D;
    }
    float den[M];
    for( int k=0; k<M; k++ ) den[k] = W[k].Denom();   // R_k + inc
    if constexpr( M > 1 ) Cross( pr, g, e_o, c2, den );
    const float smw = CW::stepMax * (1.0f + CP::AGAw * agd);
    for( int k=0; k<M; k++ ) W[k].Apply( rhs[k] / den[k], -CP::Wclip, CP::Wclip, smw );   // N = 2: (NW*D)/(R+inc), Mix2's step

    // bias: dz/db = 1, d2z/db2 = 0
    b.Update( ee, e_o, e_o * c2, -CP::Bclip, CP::Bclip, 1.0f,
              CP::Config_B::stepMax * (1.0f + CP::AGAb * agd) );
  }

  // N > 2: the cross-curvature EMAs take this update's exact cross terms,
  //   d2L/dW_j dW_k = (e_o g_j)(e_o g_k) - e_o (c2 g_j g_k + d2z/dW_j dW_k),
  // the curvature term clipped and the EMA boxed as ParamUpdater does R;
  // then each denominator gets XW times its row's off-diagonal |X|.
  void Cross( const Pred& pr, const float* g, float e_o, float c2, float* den ) {
    typedef typename CP::Config_W CW;
    float off[M];
    for( int k=0; k<M; k++ ) off[k] = 0.0f;
    int q = 0;
    for( int j=0; j<M; j++ ) for( int k=j+1; k<M; k++, q++ ) {
      float d2z = -(pr.w[j] * g[k] + pr.w[k] * g[j]);
      float hc  = clip( e_o * (c2 * (g[j] * g[k]) + d2z), CW::grad2_clip );
      float x   = this->X[q] * CW::momentum_R + (e_o * g[j]) * (e_o * g[k]) - hc;
      x = clamp( x, -CW::R_clip, CW::R_clip );
      this->X[q] = x;
      off[j] += fabsf(x); off[k] += fabsf(x);
    }
    for( int k=0; k<M; k++ ) den[k] += CP::XW * off[k];
  }
};
