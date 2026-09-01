
// Process a single float. 
// Safe for the (0, 1) range.
// When called inside a loop over an array, compilers will auto-vectorize this.
inline float logf_01_autovec(float x) {
  uint32_t xi;
  // Safe, strict-aliasing compliant bit-cast. 
  // Compilers optimize this into register casts (no actual memory copy).
  // Note: If using C++20, you can replace this with std::bit_cast<uint32_t>(x);
  memcpy(&xi, &x, sizeof(xi));
  
  // 1. Extract exponent and mantissa
  int32_t exp = (int32_t)(xi >> 23) - 127;
  
  // Force the exponent to 127 (representing 1.0f) to isolate the mantissa in [1.0, 2.0)
  uint32_t mantissa_i = (xi & 0x007FFFFF) | 0x3F800000; 
  
  float mantissa;
  memcpy(&mantissa, &mantissa_i, sizeof(mantissa));
  
  // 2. Range Reduction to [sqrt(2)/2, sqrt(2))
  // Ternary operators are cleanly translated to vector blends (e.g., vblendvps)
  bool mask = (mantissa >= 1.414213562373f);
  
  exp += mask ? 1 : 0;
  mantissa *= mask ? 0.5f : 1.0f; // Halves the mantissa if >= sqrt(2)
  
  // 3. Minimax Polynomial Evaluation
  float y = mantissa - 1.0f;
  
  // Horner's method - compilers will map this directly to a chain of FMA instructions
  float p = 7.0376836292E-2f;
  p = p * y + -1.1514610310E-1f;
  p = p * y +  1.1676998740E-1f;
  p = p * y + -1.2420140846E-1f;
  p = p * y +  1.4249322787E-1f;
  p = p * y + -1.6668057665E-1f;
  p = p * y +  2.0000714765E-1f;
  p = p * y + -2.4999993993E-1f;
  p = p * y +  3.3333331174E-1f;
  
  float y2 = y * y;
  float y3 = y2 * y;
  
  // log(1+y) ~= y - 0.5*y^2 + y^3 * P(y)
  float log_mantissa = p * y3 + (y - 0.5f * y2);
  
  // 4. Final Reconstruction
  return (float)exp * 0.693147180559945f + log_mantissa;
}

// Computes 1.0f / (1.0f + expf(-x))
// Branchless, scalar, and strictly formatted for auto-vectorization.
inline float sq_autovec(float x) {
  // 1. Change base to 2: e^{-x} = 2^{-x * log2(e)}
  float z = -x * 1.4426950408889634f;
  
  // 2. Clamp z to prevent exponent overflow/underflow.
  // Bounding to [-126.0f, 126.0f] keeps it safe for float32.
  // Compilers map these ternaries directly to vector min/max instructions.
  float z_min = (z < 126.0f) ? z : 126.0f;
  float z_clamped = (z_min > -126.0f) ? z_min : -126.0f;
  
  // 3. Extract integer part (round to nearest) and fractional part.
  // The ternary offset translates to a branchless SIMD blend.
  float offset = (z_clamped < 0.0f) ? -0.5f : 0.5f;
  int32_t i = (int32_t)(z_clamped + offset);
  float f = z_clamped - (float)i;
  
  // 4. Minimax polynomial for 2^f on the range [-0.5, 0.5]
  // Compilers will map this to a chain of Fused Multiply-Adds (FMAs).
  float p = 1.33988e-3f;
  p = p * f + 9.61813e-3f;
  p = p * f + 5.550411e-2f;
  p = p * f + 2.402265e-1f;
  p = p * f + 6.931472e-1f;
  p = p * f + 1.0f;
  
  // 5. Reconstruct the exponential: e^{-x} = 2^i * 2^f
  // We compute 2^i by directly injecting 'i' into the float exponent bits.
  uint32_t exp_bits = (uint32_t)(i + 127) << 23;
  float pow2_i;
  memcpy(&pow2_i, &exp_bits, sizeof(pow2_i));
  
  float exp_nx = pow2_i * p;
  
  // 6. Final Sigmoid computation
  return 1.0f / (1.0f + exp_nx);
}

inline constexpr float SIGMOID_CLIP = 5.95f;
inline float sq_pade(float x) {
  if (x > SIGMOID_CLIP) return 0.9974f;
  if (x < -SIGMOID_CLIP) return 0.0026f;
  float u = 0.5f * x, u2 = u * u;
  return 0.5f * (1.0f + (u * (27.0f + u2)) / (27.0f + 9.0f * u2));
}


//inline constexpr float SIGMOID_CLIP = 5.95f;

inline float sq_pade_branchless(float x) {
  constexpr float SIGMOID_CLIP = 9.95f;
  // 1. Branchless input clamp. 
  // Prevents the O(x^3) / O(x^2) rational function from diverging on huge inputs.
  float cx = x > SIGMOID_CLIP ? SIGMOID_CLIP : x;
  cx = cx < -SIGMOID_CLIP ? -SIGMOID_CLIP : cx;

  float u = 0.5f * cx;
  float u2 = u * u;

  // 2. Polynomial evaluation
  float val = 0.5f * (1.0f + (u * (27.0f + u2)) / (27.0f + 9.0f * u2));

  // 3. Branchless output clamp.
  // Your original formula hits 1.0 at the edges; this restores your specific bounds.
  val = val > 0.9974f ? 0.9974f : val;
  val = val < 0.0026f ? 0.0026f : val;
  
  return val;
}

inline float sq_pade_precise(float x) {
  // 1. Branchless clamp (expanded slightly to 7.0f as the curve holds better)
  float cx = x > 7.0f ? 7.0f : x;
  cx = cx < -7.0f ? -7.0f : cx;

  float x2 = cx * cx;

  // 2. FMA-optimized Pade (5,4) evaluation
  // Compilers will aggressively chain these into vfmadd231ps instructions
  float num = cx * (105.0f + 2.5f * x2);
  
  // Denominator factored to use FMA: 210 + x^2 * (22.5 + 0.125 * x^2)
  float den = 210.0f + x2 * (22.5f + 0.125f * x2);

  return 0.5f + 0.5f * (num / den);
}

inline float sq_pade_branchless_tuned(float x) {
  // 1. Branchless clamp to [-6.0, 6.0]
  float cx = x > 6.0f ? 6.0f : x;
  cx = cx < -6.0f ? -6.0f : cx;
  
  float x2 = cx * cx;
  
  // 2. Folded scaling and Minimax optimized coefficients.
  // Mathematically guaranteed to hit exactly 1.0 at the x=6.0 clamp limit.
  // Saves 1 multiplication compared to the original u = 0.5f * cx approach.
  return 0.5f * (1.0f + (cx * (76.8f + x2)) / (153.6f + 14.533333f * x2));
}

inline float sq_pade_precise_fast(float x) {
  // 1. Branchless clamp to [-7.0, 7.0]
  float cx = x > 7.0f ? 7.0f : x;
  cx = cx < -7.0f ? -7.0f : cx;
  
  float x2 = cx * cx;
  
  // 2. Multiplied by 8 to eliminate the 0.125f float multiplication.
  // (180.0f + x2) is now evaluated via a simple Add instruction, 
  // saving 1 CPU cycle of latency over the previous FMA instruction.
  float num = cx * (840.0f + 20.0f * x2);
  float den = 1680.0f + x2 * (180.0f + x2);
  
  return 0.5f + 0.5f * (num / den);
}

inline float sq_autovec_tuned(float x) {
  // 1. Change base to 2: e^{-x} = 2^{-x * log2(e)}
  float z = -x * 1.4426950408889634f;
  
  // 2. Branchless Clamp
  float z_min = (z < 126.0f) ? z : 126.0f;
  float z_clamped = (z_min > -126.0f) ? z_min : -126.0f;
  
  // 3. Extract integer and fractional parts
  float offset = (z_clamped < 0.0f) ? -0.5f : 0.5f;
  int32_t i = (int32_t)(z_clamped + offset);
  float f = z_clamped - (float)i;
  
  // 4. Degree-2 Minimax for 2^f (Saves 3 FMAs over previous version)
  // The middle is slightly jagged, but the exponential tails are perfect.
  float p = 0.2402f;
  p = p * f + 0.6931f;
  p = p * f + 1.0f;
  
  // 5. Reconstruct 2^i
  uint32_t exp_bits = (uint32_t)(i + 127) << 23;
  float pow2_i;
  memcpy(&pow2_i, &exp_bits, sizeof(pow2_i));
  
  // 6. Final Sigmoid
  return 1.0f / (1.0f + pow2_i * p);
}

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
