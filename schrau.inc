// schrau.inc -- Schraudolph's bit-trick exp/log/sigmoid/stretch (N. N.
// Schraudolph, "A fast, compact approximation of the exponential function",
// 1999) and macros that put them in place of libm's.  coder0.cpp includes
// it right after clamp(), so the init-time rt_logf/rt_expf are replaced too.
// Piecewise-linear in the mantissa: up to ~4% relative error on exp, ~0.06
// absolute on log.  Measurements: SSE-DESIGN.md sec.6.14.

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

// Only logf/expf are redirected.  The sigmoid and the stretch are called by
// name: Counter::sq builds its own from expf, and Counter::st calls
// ::st_schraudolph directly -- macros for them would rewrite those two
// member names instead, which is what the earlier sq/st defines did.
#define logf(x) logf_schraudolph(x)
#define expf(x) expf_schraudolph(x)
