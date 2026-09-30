// schrau.inc -- Schraudolph's bit-trick exp/log/sigmoid/stretch (N. N.
// Schraudolph, "A fast, compact approximation of the exponential function",
// 1999) and macros that put them in place of libm's.  coder0.cpp includes
// it with -DUSE_SCHRAU=1 (after clamp(): the init-time rt_logf/rt_expf are
// replaced too) or -DUSE_SCHRAU=2 (after them: seeds and box edges stay
// exact).  Piecewise-linear in the mantissa: up to ~4% relative error on
// exp, ~0.06 absolute on log.  Measurements: SSE-DESIGN.md sec.6.14.

inline float sq_schraudolph(float x) {
  // 1. Branchless clamp to prevent float exponent overflow/underflow
//  float cx = x < -87.3f ? -87.3f : x;
//  cx = cx > 87.3f ? 87.3f : cx;
  float cx = clip(x,87.3f);
  
  // 2. The Magic Schraudolph Constant for e^{-x}
  // This mathematically aligns the float representation to base-2 exponentiation
  int32_t i = (int32_t)(-cx * 12102203.16f + 1064866805.0f);
  
  float exp_nx;
  memcpy(&exp_nx, &i, sizeof(exp_nx));
  
  // 3. Final Sigmoid
  return 1.0f / (1.0f + exp_nx);
}

inline float logf_schraudolph(float x) {
  // 1. Clamp to prevent log(<= 0)
  // 1.175494e-38f is roughly FLT_MIN (the smallest positive normalized float)
  float cx = x < 1.175494e-38f ? 1.175494e-38f : x;

  // 2. Read the float's IEEE-754 bits directly as an integer
  int32_t i;
  memcpy(&i, &cx, sizeof(i));

  // 3. The Reverse Magic Schraudolph Constants
  // ln(x) = i * (1 / 12102203.16) - (1064866805 / 12102203.16)
  return (float)i * 8.262958288e-8f - 87.989971088f;
}


inline float expf_schraudolph(float x) {
  // 1. Branchless clamp to prevent float exponent overflow/underflow
  // 87.3f is roughly the threshold where standard 32-bit floats overflow to infinity.
  float cx = x < -87.3f ? -87.3f : x;
  cx = cx > 87.3f ? 87.3f : cx;
  
  // $2^{23}$ shifts the value into the exponent field of the IEEE-754 float
  // Dividing by $\ln(2)$ converts the natural exponential $e^x$ into a base-2 exponential $2^x$, 
  // which the floating-point hardware natively understands via its exponent bits.

  // 2. The Magic Schraudolph Constant for e^{x}
  // i ? x * (2^23 / ln(2)) + B
  int32_t i = (int32_t)(cx * 12102203.16f + 1064866805.0f);
  // B constant 1064866805.0f handles the IEEE-754 exponent bias (127 * 2^23), slightly tuned downward to center the piecewise linear error.
  
  // 3. Bit-cast back to float
  float exp_x;
  memcpy(&exp_x, &i, sizeof(exp_x));
  
  return exp_x;
}

inline float st_schraudolph(float p) {
  // stretch ln(p/(1-p)) = ln(p) - ln(1-p): with Schraudolph's log both terms
  // are (bits * 8.26e-8 - 87.99), so the bias constants cancel and the
  // stretch is one integer subtraction and one multiply, no division.
  // 1. Clamp p away from 0 and 1 (log(<= 0)); FLT_MIN as in logf_schraudolph
  float cp = p < 1.175494e-38f ? 1.175494e-38f : p;
  float cq = 1.0f - p;
  cq = cq < 1.175494e-38f ? 1.175494e-38f : cq;

  // 2. The two floats' IEEE-754 bits
  int32_t ip, iq;
  memcpy(&ip, &cp, sizeof(ip));
  memcpy(&iq, &cq, sizeof(iq));

  // 3. ln(p) - ln(1-p) = (ip - iq) * (ln 2 / 2^23)
  return (float)(ip - iq) * 8.262958288e-8f;
}

inline float tanhf_schraudolph(float x) {
  // tanh(x) = (e^x - e^-x) / (e^x + e^-x).  The bit patterns of e^x and e^-x
  // sum to 2*B, so e^-x costs one integer subtraction, not a second multiply.
  // A common scale factor of the pair cancels in the ratio, so B only sets the
  // phase of the piecewise-linear mantissa error: retuned for the ratio,
  // max |err| 0.0031 (expf's 1064866805 would give 0.0103 here), tanh(0)=0,
  // odd to within one float step of the conversion.
  // 1. Branchless clamp; at +-87.3 both exponentials are still normal floats
  float cx = clip(x,87.3f);

  // 2. The Magic Schraudolph Constants for e^{x} and e^{-x}
  // B = 1065217024 = (127<<23) - 136192;  2*B = 2130434048
  int32_t ip = (int32_t)(cx * 12102203.16f + 1065217024.0f);
  int32_t in = 2130434048 - ip;

  // 3. Bit-cast back to float
  float ep, en;
  memcpy(&ep, &ip, sizeof(ep));
  memcpy(&en, &in, sizeof(en));

  // 4. Final tanh
  return (ep - en) / (ep + en);
}
#define SCHRAUDOLPH 1
#define logf(x) logf_schraudolph(x)
#define expf(x) expf_schraudolph(x)
#define sq(x)   sq_schraudolph(x)
#define st(x)   st_schraudolph(x)
#define tanhf(x) tanhf_schraudolph(x)
