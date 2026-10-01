// =========================================================================

static inline float clip(float x, float d) {
  return fminf(fmaxf(x, -d), d);
}

static inline float clamp(float x, float min_val, float max_val) {
  return fminf(fmaxf(x, min_val), max_val);
}

#include "logf1.inc"

static inline float DotProduct(cprfloat x1, cprfloat x2, uint len) {
  float sum = 0;
  for( uint i = 0; i<len; i++ )
    sum += x1[i]*x2[i];
  return sum;
}

static inline float SumOfSquares(cprfloat a, uint len) {
  float sum = 0;
  for( uint i = 0; i<len; i++ )
    sum += a[i]*a[i];
  return sum;
}

static inline float SumOfAbs(cprfloat a, uint len) {
  float sum = 0;
  for( uint i = 0; i<len; i++ )
    sum += fabs(a[i]);
  return sum;
}

static inline float MaxOfAbs(cprfloat a, uint len) {
  float r = 0, b;
  for( uint i = 0; i<len; i++ ) {
    b = fabs(a[i]);
    r = r<b ? b : r;
  }
  return r;
}

static inline void ClipArray(prfloat a, uint len, const float K) {
  for( uint i = 0; i<len; i++ )
    a[i] = a[i]>K ? K : (a[i]<-K ? -K : a[i]);
}

static inline float Rand() {
  return static_cast<float>(rand())/static_cast<float>(RAND_MAX);
}


static inline float st(const float p_) {
  float p = p_ * 0.999998f + 0.000001f;
  return logf(p/(1.0f-p));
}

static inline float sq(const float x) {
  return 1.0f/(1.0f+expf(-x));
}

//#define logf(x) logf_schraudolph(x)
//#define expf(x) expf_schraudolph(x)
//#define sq(x)   sq_schraudolph(x)
