/* prog64c.c -- floating-point x86-64 that gcc -O2 lowers to SSE2: movsd/movapd,
 * addsd/subsd/mulsd/divsd, sqrtsd, ucomisd/comisd, cvtsi2sd/cvttsd2si, pxor
 * (zeroing), and the integer scaffolding (endbr, call, jcc). Validates the SSE2
 * subset added to corpus64.p round-trips on real compiler output. */
double poly(double x, double a, double b, double c){ return a*x*x + b*x + c; }
double norm(double x, double y, double z){ return __builtin_sqrt(x*x + y*y + z*z); }
double scale(const double *v, int n, double k){ double s = 0; for (int i=0;i<n;i++) s += v[i]*k; return s; }
int    cmp(double a, double b){ if (a < b) return -1; return a > b ? 1 : 0; }
double i2d(long n){ return (double)n + 0.5; }
long   d2i(double x){ return (long)(x * 16.0); }
double recip(double x){ return 1.0 / x; }
float  fmul(float a, float b){ return a * b + 1.0f; }
