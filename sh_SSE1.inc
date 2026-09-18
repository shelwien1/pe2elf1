
const int hSCALE = SCALE/2;
const int mSCALE = SCALE-1;

// ==========================================
// Float-based Secondary Symbol Estimation
// ==========================================

template<int SSEQuant=7, int SSEInter=SSEQuant, int Width=0>
struct SSEii {
  enum { Q=SSEQuant, I=SSEInter };

  float sse_tot[1<<Width][SSEInter];
  float sse_mpc[1<<Width][SSEQuant];
  uint  ssebits;

  // Per-query saved state, valid between Step() and Update().
  float saved_P;
  float saved_sw;
  int   saved_q;
  int   saved_t;
  int   saved_m;

  static constexpr float kMaxCount = 550.0f;        // asymptotic 1/N learning rate
  static constexpr float kEps      = 1.0f/32768.0f; // matches old SCALE quantum
  static constexpr float K_stretch = 0.25f;         // rational stretch constant

  void Init(float Wi = 0.06f, float Ti = 126.0f) {
    ssebits = 0;
    for( int j=0; j<(1<<Width); j++ ) {
      for( int i=0; i<SSEQuant; i++ ) {
        // Linear index domain -> Stretch domain mapping
        float P_mapped = float(i) / float(SSEQuant-1);
        float P = (P_mapped * K_stretch) / (1.0f + K_stretch - P_mapped);
        sse_mpc[j][i] = (1.0f-Wi)*P + 0.5f*Wi;
      }
      for( int i=0; i<SSEInter; i++ ) sse_tot[j][i] = Ti;
    }
  }

  void Update(float freq_ratio, float& remaining, uint& freq_out) {
    float p_raw = 1.0f - freq_ratio;
    if( p_raw < kEps      ) p_raw = kEps;
    if( p_raw > 1.0f-kEps ) p_raw = 1.0f - kEps;

    // Rational stretch to heavily expand resolution near p_raw = 0
    float p_mapped = p_raw / (p_raw + K_stretch) * (1.0f + K_stretch);

    const float q_freq = (SSEQuant-1) * p_mapped;
    const float q_totl = (SSEInter-1) * p_mapped;
    int   qbin = int(q_freq); if( qbin > SSEQuant-2 ) qbin = SSEQuant-2;
    int   tbin = int(q_totl); if( tbin > SSEInter-1 ) tbin = SSEInter-1;
    const int   mbin = ssebits & ((1<<Width)-1);
    const float sw   = q_freq - qbin;

    const float C0 = sse_mpc[mbin][qbin];
    const float C1 = sse_mpc[mbin][qbin+1];
    float P = C0 + sw*(C1-C0);
    if( P < kEps      ) P = kEps;
    if( P > 1.0f-kEps ) P = 1.0f - kEps;

    saved_P  = P;
    saved_sw = sw;
    saved_q  = qbin;
    saved_t  = tbin;
    saved_m  = mbin;

    const float p_is = 1.0f - P;
    freq_out   = uint(remaining * p_is * float(SCALE));
    remaining *= P;
  }

  void Update(byte c) {
    const int m = saved_m;

    float t0 = sse_tot[m][saved_t] + 1.0f;
    if( t0 > kMaxCount ) t0 = kMaxCount;
    const float wr = 1.0f / t0;
    sse_tot[m][saved_t] = t0;

    float P = saved_P * (1.0f - wr);
    if( c==0 ) P += wr;
    ssebits = (ssebits<<1) | (c&1);

    // Mathematically optimal normalized least-squares gradient update
    const float delta = P - saved_P;
    const float w0    = 1.0f - saved_sw;
    const float w1    = saved_sw;
    const float norm  = 1.0f / (w0*w0 + w1*w1);

    sse_mpc[m][saved_q]   += delta * w0 * norm;
    sse_mpc[m][saved_q+1] += delta * w1 * norm;

    //const float dS = S0 - S1;
    //const float sw_dS = saved_sw * dS;
    //sse_mpc[m][saved_q]   += delta;
    //sse_mpc[m][saved_q+1] += delta;
  }
};

static SSEii<8> SSE[256*256];

