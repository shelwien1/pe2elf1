/* prog64f.c -- long double math, which gcc lowers to x87 (D8-DF): fldt/fstpt
 * (80-bit load/store), fmul/fadd/fmulp, fxch, fucomi/fcomi, fildll/fistpll, the
 * control ops. Validates the x87 escape opcodes round-trip on real compiler
 * output. (x86-64 uses x87 only for long double; SSE handles float/double.) */
long double area(long double r){ return 3.14159265358979L * r * r; }
long double sumld(const long double *a, int n){ long double s=0; for(int i=0;i<n;i++) s+=a[i]; return s; }
int         cmpld(long double a, long double b){ return a < b ? -1 : a > b ? 1 : 0; }
long double powi(long double x, int n){ long double r=1; while(n-->0) r*=x; return r; }
long double fromll(long long n){ return (long double)n + 0.5L; }
